/*
 * Copyright (c) 2025, Alibaba Group Holding Limited;
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <iterator>
#include <span>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>

#include "asio/buffer.hpp"
#include "async_simple/coro/Lazy.h"
#include "ylt/coro_io/coro_io.hpp"
#include "nd_buffer_pool.hpp"
#include "nd_connector.hpp"
#include "nd_listener.hpp"
#include "nd_socket.hpp"
#include "nd_tcp.hpp"

// Free coroutine functions over nd_socket_t, mirroring ib_io.hpp. All return
// async_simple::coro::Lazy<...> and bridge the ND backend's asio-handler async
// ops via coro_io::callback_awaitor.
namespace coro_io {

// Accept the next incoming connection on `listener` into `nd_socket`.
// (ND analogue of ib's async_accept(acceptor, ib_socket); takes the native ND
// listener instead of a TCP acceptor -- ND needs no TCP handshake.)
inline async_simple::coro::Lazy<std::error_code> async_accept(
    nd_listener<tcp>& listener, nd_socket_t& nd_socket) {
  auto& io_ctx = nd_socket.get_state()->io_ctx_;
  nd_connector<tcp> conn(io_ctx);
  std::array<std::byte, nd_socket_t::private_data_size> req_pd{};

  coro_io::callback_awaitor<std::pair<std::error_code, std::size_t>> awaitor;
  auto [ec, n] = co_await awaitor.await_resume([&](auto handler) {
    listener.async_get_connection(
        conn, asio::buffer(req_pd),
        [handler](asio::error_code ec, std::size_t n) mutable {
          handler.set_value_then_resume(std::pair{std::error_code(ec), n});
        });
  });
  if (ec) [[unlikely]] {
    co_return ec;
  }
  auto ret = co_await nd_socket.accept(
      std::move(conn), std::span<const std::byte>{req_pd.data(), n});
  if (ret) [[unlikely]] {
    nd_socket.close();
    co_return std::make_error_code(std::errc::protocol_error);
  }
  co_return ret;
}

inline async_simple::coro::Lazy<std::error_code> async_connect(
    nd_socket_t& nd_socket, const std::string& host, const std::string& port) {
  return nd_socket.connect(host, port);
}

template <typename EndPointSeq>
inline async_simple::coro::Lazy<std::error_code> async_connect(
    nd_socket_t& nd_socket, const EndPointSeq& endpoint) noexcept {
  return nd_socket.connect(endpoint);
}

namespace detail {

template <typename Buffer>
inline bool has_gpu_buffer(const Buffer& buffer) {
  if constexpr (requires { buffer.gpu_id(); }) {
    return buffer.gpu_id() >= 0;
  }
  else if constexpr (requires {
                       std::begin(buffer);
                       std::end(buffer);
                     }) {
    bool has_gpu = false;
    for (auto const& item : buffer) {
      if constexpr (requires { item.gpu_id(); }) {
        has_gpu = has_gpu || item.gpu_id() >= 0;
      }
    }
    return has_gpu;
  }
  else {
    return false;
  }
}

template <typename Buffer>
inline std::size_t buffer_size(const Buffer& buffer) {
  if constexpr (requires { buffer.gpu_id(); }) {
    return buffer.size();
  }
  else {
    return asio::buffer_size(buffer);
  }
}

template <typename Buffer>
inline std::size_t copy_from_buffer_sequence(const Buffer& buffer,
                                             std::size_t offset, char* dst,
                                             std::size_t len) {
  if constexpr (requires { buffer.gpu_id(); }) {
    if (offset >= buffer.size()) {
      return 0;
    }
    auto n = (std::min)(len, buffer.size() - offset);
    std::memcpy(dst, buffer.data() + offset, n);
    return n;
  }
  else {
    std::size_t copied = 0;
    for (auto it = asio::buffer_sequence_begin(buffer),
              end = asio::buffer_sequence_end(buffer);
         it != end && copied < len; ++it) {
      auto const* src = static_cast<const char*>(it->data());
      auto size = it->size();
      if (offset >= size) {
        offset -= size;
        continue;
      }
      auto n = (std::min)(len - copied, size - offset);
      std::memcpy(dst + copied, src + offset, n);
      copied += n;
      offset = 0;
    }
    return copied;
  }
}

template <typename Buffer>
inline std::size_t consume_to_buffer_sequence(nd_socket_t& nd_socket,
                                              Buffer&& buffer,
                                              std::size_t offset,
                                              std::size_t len) {
  if constexpr (requires { buffer.gpu_id(); }) {
    if (offset >= buffer.size()) {
      return 0;
    }
    auto n = (std::min)(len, buffer.size() - offset);
    return nd_socket.consume(buffer.mutable_data() + offset, n);
  }
  else {
    std::size_t copied = 0;
    for (auto it = asio::buffer_sequence_begin(buffer),
              end = asio::buffer_sequence_end(buffer);
         it != end && copied < len; ++it) {
      auto* dst = static_cast<char*>(it->data());
      auto size = it->size();
      if (offset >= size) {
        offset -= size;
        continue;
      }
      auto n = (std::min)(len - copied, size - offset);
      auto got = nd_socket.consume(dst + offset, n);
      copied += got;
      offset = 0;
      if (got < n) {
        break;
      }
    }
    return copied;
  }
}

}  // namespace detail

template <typename buffer_t>
async_simple::coro::Lazy<std::pair<std::error_code, std::size_t>> async_write(
    nd_socket_t& nd_socket, buffer_t&& buffer) {
  co_await coro_io::dispatch(nd_socket.get_executor());

  if (detail::has_gpu_buffer(buffer)) [[unlikely]] {
    co_return std::pair{std::make_error_code(std::errc::not_supported),
                        std::size_t{0}};
  }

  std::size_t total = detail::buffer_size(buffer);
  std::size_t chunk = nd_socket.get_buffer_size();
  auto pool = nd_socket.buffer_pool();

  std::error_code ec{};
  std::size_t sent_total = 0;
  while (sent_total < total) {
    if (!nd_socket.is_open()) [[unlikely]] {
      ec = std::make_error_code(std::errc::not_connected);
      break;
    }
    std::size_t n = (std::min)(chunk, total - sent_total);
    auto send_buffer = pool->get_buffer();
    if (!send_buffer) [[unlikely]] {
      ec = std::make_error_code(std::errc::no_buffer_space);
      break;
    }
    auto copied = detail::copy_from_buffer_sequence(
        buffer, sent_total, static_cast<char*>(send_buffer.data()), n);
    if (copied != n) [[unlikely]] {
      ec = std::make_error_code(std::errc::invalid_argument);
      break;
    }
    auto [sec, sn] = co_await nd_socket.async_send_one(
        send_buffer.const_view(std::size_t{0}, n));
    if (sec) [[unlikely]] {
      ec = sec;
      break;
    }
    sent_total += sn;
    // send_buffer returns to the pool at end of scope.
  }
  co_return std::pair{ec, sent_total};
}

namespace detail {

template <typename buffer_t>
async_simple::coro::Lazy<std::pair<std::error_code, std::size_t>>
nd_async_read_impl(nd_socket_t& nd_socket, buffer_t&& buffer, bool read_some) {
  co_await coro_io::dispatch(nd_socket.get_executor());

  if (detail::has_gpu_buffer(buffer)) [[unlikely]] {
    co_return std::pair{std::make_error_code(std::errc::not_supported),
                        std::size_t{0}};
  }

  std::size_t total = detail::buffer_size(buffer);
  std::size_t got = 0;
  std::error_code ec{};

  if (total == 0) [[unlikely]] {
    co_return std::pair{ec, got};
  }

  // Drain any leftover from a previous recv first.
  if (nd_socket.remain_read_buffer_size()) {
    got += consume_to_buffer_sequence(nd_socket, buffer, got, total - got);
    if (read_some && got) {
      co_return std::pair{ec, got};
    }
  }

  while (got < total) {
    auto [rec, n] = co_await nd_socket.async_recv_one();
    if (rec) [[unlikely]] {
      ec = rec;
      break;
    }
    if (n == 0) [[unlikely]] {
      break;  // peer closed
    }
    got += consume_to_buffer_sequence(nd_socket, buffer, got, total - got);
    if (read_some) {
      break;
    }
  }
  co_return std::pair{ec, got};
}

}  // namespace detail

template <typename buffer_t>
async_simple::coro::Lazy<std::pair<std::error_code, std::size_t>> async_read(
    nd_socket_t& nd_socket, buffer_t&& buffer) {
  return detail::nd_async_read_impl(nd_socket, std::forward<buffer_t>(buffer),
                                    false);
}

template <typename buffer_t>
async_simple::coro::Lazy<std::pair<std::error_code, std::size_t>>
async_read_some(nd_socket_t& nd_socket, buffer_t&& buffer) {
  return detail::nd_async_read_impl(nd_socket, std::forward<buffer_t>(buffer),
                                    true);
}

}  // namespace coro_io
