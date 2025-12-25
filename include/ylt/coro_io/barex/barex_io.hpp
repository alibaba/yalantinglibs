#pragma once

#define CMAKE_INCLUDE
#include <system_error>

#include "barex_socket.hpp"
#include "ylt/coro_io/barex/barex_acceptor.hpp"
#include "ylt/easylog.hpp"
namespace coro_io {
inline async_simple::coro::Lazy<std::error_code> async_accept(
    barex_acceptor_t& acceptor, coro_io::barex_socket_t& barex_socket) {
  auto impl = co_await acceptor->accept();
  if (impl == nullptr) {
    co_return std::make_error_code(std::errc::operation_canceled);
  }
  barex_socket = coro_io::barex_socket_t{std::move(impl)};
  co_return std::error_code{};
}
inline async_simple::coro::Lazy<std::error_code> async_connect(
    coro_io::barex_socket_t& barex_socket, const std::string& host,
    const std::string& port) {
  return barex_socket.get_impl()->connect(host, port);
}
template <typename EndPointSeq>
inline async_simple::coro::Lazy<std::error_code> async_connect(
    coro_io::barex_socket_t& barex_socket,
    const EndPointSeq& endpoint) noexcept {
  return barex_socket.get_impl()->connect(endpoint);
}

namespace detail {

inline std::size_t consume_buffer(coro_io::barex_socket_t& barex_socket,
                                  std::vector<std::span<char>>& dst) {
  std::size_t total = 0;
  for (auto iter = dst.begin(); iter != dst.end(); ++iter) {
    auto& e = *iter;
    while (e.size()) {
      ELOG_TRACE << "consume_buffer: ";
      std::size_t len = barex_socket.get_impl()->consume_buffer(e);
      if (len == 0) {
        dst = std::vector<std::span<char>>{iter, dst.end()};
        return total;
      }
      total += len;
    }
  }
  dst = {};
  return total;
};

template <typename T>
std::vector<std::span<char>> make_barex_buffer(T& buffer) {
  using pointer_t = decltype(buffer.data());
  if constexpr (std::is_same_v<pointer_t, void*> ||
                std::is_same_v<pointer_t, char*> ||
                std::is_same_v<pointer_t, const char*>) {
    return std::vector{std::span<char>{(char*)buffer.data(), buffer.size()}};
  }
  else {
    std::vector<std::span<char>> ret;
    ret.reserve(buffer.size());
    for (auto& e : buffer) {
      ret.emplace_back(e.data(), e.size());
    }
    return ret;
  }
}

inline std::vector<std::span<char>> make_split_buffer(
    std::vector<std::span<char>>& buffer, std::size_t split_size) {
  assert(split_size > 0);
  std::vector<std::span<char>> result;
  result.reserve(buffer.size());
  for (auto& span : buffer) {
    std::size_t remaining = span.size();
    char* ptr = span.data();
    while (remaining > split_size) {
      result.emplace_back(ptr, split_size);
      ptr += split_size;
      remaining -= split_size;
    }

    if (remaining > 0) {
      result.emplace_back(ptr, remaining);
    }
  }
  return result;
}

template <ib_socket_t::io_type io, typename Buffer>
async_simple::coro::Lazy<std::pair<std::error_code, std::size_t>> async_io_impl(
    coro_io::barex_socket_t& barex_socket, Buffer&& raw_buffer,
    bool read_some = false) {
  int cnt = 0;
  if (!barex_socket.get_executor().running_in_this_thread()) {
    // switch to io_thread
    co_await dispatch(barex_socket.get_executor());
  }
  std::size_t io_completed_size = 0;
  std::vector<std::span<char>> buffer = make_barex_buffer(raw_buffer);
  if constexpr (io == ib_socket_t::io_type::recv) {
    if (buffer.size()) {
      ELOG_TRACE << "ready recv, len:" << buffer[0].size();
    }
    io_completed_size = consume_buffer(barex_socket, buffer);
    if (buffer.empty() || (io_completed_size && read_some)) {
      co_return std::pair{std::error_code{}, io_completed_size};
    }
  }
  if (barex_socket.has_closed()) {
    co_return std::make_pair(
        std::make_error_code(std::errc::operation_canceled), 0);
  }
  if constexpr (io == ib_socket_t::io_type::recv) {
    if (buffer.size()) {
      ELOG_TRACE << "ready recv waiting, len:" << buffer[0].size();
    }
    auto [ec, len] =
        co_await coro_io::async_io<std::pair<std::error_code, std::size_t>>(
            [read_some, buffer = std::move(buffer),
             &barex_socket](auto&& callback) mutable {
              barex_socket.get_impl()->post_recv(std::move(buffer), read_some,
                                                 std::move(callback));
            },
            barex_socket);
    ELOG_TRACE << "ready recv waiting, len:" << len << ",ec:" << ec.message();
    io_completed_size += len;
    co_return std::pair{ec, io_completed_size};
  }
  else {
    if (buffer.size()) {
      ELOG_TRACE << "ready send, len:" << buffer[0].size();
    }
    buffer =
        make_split_buffer(buffer, barex_socket.get_impl()->get_buffer_size());
    std::size_t len = 0;
    std::error_code ec;
    for (auto& e : buffer) {
      if (barex_socket.get_impl()->is_send_queue_full()) {
        ELOG_INFO << "waiting write over";
        ec = co_await barex_socket.get_impl()->waiting_write_over();
        ELOG_INFO << "write over";
        if (ec) {
          break;
        }
      }
      ++cnt;
      ELOG_INFO << "post send, cnt:" << cnt
                << ",this:" << (void*)barex_socket.get_impl().get();
      ec = barex_socket.get_impl()->post_send(e);
      if (ec) {
        break;
      }
      len += e.size();
    }
    if (ec) {
      barex_socket.close();
    }
    co_return std::pair{ec, len};
  }
}
}  // namespace detail

template <typename buffer_t>
async_simple::coro::Lazy<std::pair<std::error_code, std::size_t>> async_write(
    coro_io::barex_socket_t& barex_socket, buffer_t&& buffer) {
  return detail::async_io_impl<ib_socket_t::send>(
      barex_socket, std::forward<buffer_t>(buffer));
}

template <typename buffer_t>
async_simple::coro::Lazy<std::pair<std::error_code, std::size_t>> async_read(
    coro_io::barex_socket_t& barex_socket, buffer_t&& buffer) {
  return detail::async_io_impl<ib_socket_t::recv>(
      barex_socket, std::forward<buffer_t>(buffer));
}

template <typename buffer_t>
async_simple::coro::Lazy<std::pair<std::error_code, std::size_t>>
async_read_some(coro_io::barex_socket_t& barex_socket, buffer_t&& buffer) {
  auto result = co_await detail::async_io_impl<ib_socket_t::recv>(
      barex_socket, std::forward<buffer_t>(buffer), true);
  co_return result;
}
};  // namespace coro_io