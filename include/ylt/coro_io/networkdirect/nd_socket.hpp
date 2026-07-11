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
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <system_error>
#include <utility>

#include "asio/buffer.hpp"
#include "asio/dispatch.hpp"
#include "asio/io_context.hpp"
#include "asio/ip/address.hpp"
#include "async_simple/coro/Lazy.h"
#include "async_simple/util/move_only_function.h"
#include "nd_buffer.hpp"
#include "nd_buffer_pool.hpp"
#include "nd_connector.hpp"
#include "nd_device.hpp"
#include "nd_error.hpp"
#include "nd_listener.hpp"
#include "nd_queue_pair.hpp"
#include "nd_tcp.hpp"
#include "ylt/coro_io/coro_io.hpp"
#include "ylt/coro_io/io_context_pool.hpp"
#include "ylt/easylog.hpp"

namespace coro_io {

namespace detail {

// Shared state of an nd_socket. Held by shared_ptr so completion tokens (which
// run on the io_context CQ-poller thread) can keep it alive. Mirrors
// ib_socket_shared_state_t, but ND has no manual completion-channel event loop:
// each posted send/recv carries its own token, invoked by use_device's shared
// IOCP CQ poller. RDMA completions are in-order per QP, so posted recv buffers
// are tracked FIFO.
struct nd_socket_shared_state_t
    : public std::enable_shared_from_this<nd_socket_shared_state_t> {
  using callback_t = async_simple::util::move_only_function<void(
      std::pair<std::error_code, std::size_t>)>;

  struct recv_result_t {
    std::error_code ec;
    std::size_t len = 0;
    nd_buffer_t buf;
  };

  coro_io::ExecutorWrapper<>* executor_;
  asio::io_context& io_ctx_;
  nd_device_ptr device_;
  std::shared_ptr<nd_buffer_pool_t> pool_;
  nd_queue_pair qp_;
  std::optional<nd_connector<tcp>> conn_;
  std::uint16_t recv_buffer_cnt_;

  // recv pipeline (all touched on the io thread)
  std::deque<nd_buffer_t> posted_recv_bufs_;  // FIFO, matches completion order
  std::deque<recv_result_t> recv_results_;    // completed, not yet delivered
  callback_t recv_cb_;                        // pending user recv
  std::size_t outstanding_recv_ = 0;

  // the recv buffer currently being consumed by the io layer
  nd_buffer_t current_recv_;
  std::size_t recv_off_ = 0;
  std::size_t recv_len_ = 0;

  std::atomic<bool> has_close_ = false;

  nd_socket_shared_state_t(coro_io::ExecutorWrapper<>* executor,
                           nd_device_ptr device,
                           std::shared_ptr<nd_buffer_pool_t> pool,
                           std::uint16_t recv_buffer_cnt)
      : executor_(executor),
        io_ctx_(static_cast<asio::io_context&>(executor->context())),
        device_(std::move(device)),
        pool_(std::move(pool)),
        qp_(io_ctx_),
        recv_buffer_cnt_(recv_buffer_cnt) {}

  nd_socket_shared_state_t(nd_socket_shared_state_t&&) = delete;
  nd_socket_shared_state_t& operator=(nd_socket_shared_state_t&&) = delete;

  auto get_executor() const noexcept { return executor_->get_asio_executor(); }

  static void resume(std::pair<std::error_code, std::size_t>&& arg,
                     callback_t&& handle) {
    if (handle) [[likely]] {
      auto tmp = std::move(handle);
      tmp(std::move(arg));
    }
  }

  // Deliver one completed recv to a waiting/queued consumer: stash its buffer
  // as the current recv buffer, then resume the callback with (ec, len).
  void deliver(recv_result_t&& result, callback_t&& cb) {
    current_recv_ = std::move(result.buf);
    recv_off_ = 0;
    recv_len_ = result.len;
    resume(std::pair{result.ec, result.len}, std::move(cb));
  }

  // Keep recv_buffer_cnt_ recv WRs outstanding (called on the io thread).
  void arm_recv_buffers() {
    while (!has_close_ && outstanding_recv_ < recv_buffer_cnt_) {
      auto buffer = pool_->get_buffer();
      if (!buffer) {
        ELOG_WARN << "nd_socket: out of recv buffers";
        break;
      }
      auto view = buffer.mutable_view();
      posted_recv_bufs_.push_back(std::move(buffer));
      ++outstanding_recv_;
      auto self = shared_from_this();
      qp_.async_recv(view, [self](asio::error_code ec, std::size_t n) {
        self->on_recv_complete(ec, n);
      });
    }
  }

  void on_recv_complete(std::error_code ec, std::size_t len) {
    if (outstanding_recv_) {
      --outstanding_recv_;
    }
    recv_result_t result;
    result.ec = ec;
    result.len = len;
    if (!posted_recv_bufs_.empty()) {
      result.buf = std::move(posted_recv_bufs_.front());
      posted_recv_bufs_.pop_front();
    }

    if (recv_cb_) {
      deliver(std::move(result), std::move(recv_cb_));
    }
    else {
      recv_results_.push_back(std::move(result));
    }

    if (!ec && !has_close_) {
      arm_recv_buffers();
    }
  }

  void post_recv_impl(callback_t&& cb) {
    if (!recv_results_.empty()) {
      auto result = std::move(recv_results_.front());
      recv_results_.pop_front();
      deliver(std::move(result), std::move(cb));
      return;
    }
    if (has_close_) [[unlikely]] {
      resume(
          std::pair{make_error_code(rdma_errc::disconnected), std::size_t{0}},
          std::move(cb));
      return;
    }
    recv_cb_ = std::move(cb);
  }

  // Copy up to sz bytes from the current recv buffer into dst; returns bytes
  // copied. Releases the recv buffer (back to the pool) once fully consumed.
  std::size_t consume(char* dst, std::size_t sz) {
    auto remain = recv_len_ - recv_off_;
    auto len = (std::min)(sz, remain);
    if (len) {
      std::memcpy(dst, static_cast<char*>(current_recv_.data()) + recv_off_,
                  len);
      recv_off_ += len;
      if (recv_off_ == recv_len_) {
        current_recv_ = nd_buffer_t{};  // return to pool
        recv_off_ = recv_len_ = 0;
      }
    }
    return len;
  }

  std::size_t remain_read_buffer_size() const noexcept {
    return recv_len_ - recv_off_;
  }

  void cancel() {
    if (conn_) {
      asio::error_code ec;
      conn_->disconnect(ec);
    }
  }

  void close_impl() {
    if (conn_) {
      asio::error_code ec;
      conn_->disconnect(ec);
    }
    // resume any pending consumer with a disconnected error
    if (recv_cb_) {
      resume(
          std::pair{make_error_code(rdma_errc::disconnected), std::size_t{0}},
          std::move(recv_cb_));
    }
  }

  void close() {
    if (!has_close_.exchange(true)) {
      auto self = shared_from_this();
      asio::dispatch(get_executor(), [self]() {
        self->close_impl();
      });
    }
  }
};

}  // namespace detail

// Coroutine RDMA socket over NetworkDirect. Mirrors ib_socket_t: an internal
// pool of registered buffers, copy-based send/recv, connect/accept/close.
// Connection management is native (nd_connector / nd_listener) -- no TCP
// handshake. Use the nd_io free functions (async_connect / async_accept /
// async_read / async_write) for the high-level coroutine API.
class nd_socket_t {
 public:
  struct nd_socket_info {
    std::uint32_t magic = 0x594c544e;  // "YLTN"
    std::uint32_t buffer_size = 0;
    std::uint16_t port = 0;
    std::uint16_t address_size = 0;
    std::array<unsigned char, 16> address{};
  };
  constexpr static std::uint32_t nd_socket_info_magic = 0x594c544e;
  constexpr static std::size_t private_data_size = sizeof(nd_socket_info);

  struct config_t {
    std::uint32_t cq_size = 128;
    std::uint16_t recv_buffer_cnt = 8;
    std::uint16_t send_buffer_cnt = 4;
    std::size_t buffer_size = 256 * 1024;
    nd_buffer_pool_t::config_t buffer_pool_config;
    nd_device_ptr device;
  };

  using callback_t = detail::nd_socket_shared_state_t::callback_t;

  explicit nd_socket_t(
      coro_io::ExecutorWrapper<>* executor = coro_io::get_global_executor())
      : executor_(executor) {
    init(config_t{});
  }
  nd_socket_t(coro_io::ExecutorWrapper<>* executor, const config_t& config)
      : executor_(executor) {
    init(config);
  }
  explicit nd_socket_t(const config_t& config)
      : executor_(coro_io::get_global_executor()) {
    init(config);
  }

  nd_socket_t(nd_socket_t&&) = default;
  nd_socket_t& operator=(nd_socket_t&& o) {
    close();
    state_ = std::move(o.state_);
    executor_ = o.executor_;
    conf_ = std::move(o.conf_);
    buffer_size_ = o.buffer_size_;
    local_address_ = std::move(o.local_address_);
    remote_address_ = std::move(o.remote_address_);
    local_port_ = o.local_port_;
    remote_port_ = o.remote_port_;
    return *this;
  }
  ~nd_socket_t() { close(); }

  bool is_open() const noexcept {
    return state_ && state_->qp_.is_bound() && !state_->has_close_;
  }

  auto get_executor() const { return executor_->get_asio_executor(); }
  auto get_coro_executor() const { return executor_; }
  const config_t& get_config() const noexcept { return conf_; }
  std::size_t get_buffer_size() const noexcept { return buffer_size_; }
  std::shared_ptr<nd_buffer_pool_t> buffer_pool() const noexcept {
    return state_ ? state_->pool_ : nullptr;
  }
  std::shared_ptr<detail::nd_socket_shared_state_t> get_state() const noexcept {
    return state_;
  }
  void set_local_endpoint(asio::ip::address address,
                          std::uint16_t port) noexcept {
    local_address_ = std::move(address);
    local_port_ = port;
  }
  asio::ip::address get_remote_address() const noexcept {
    return remote_address_;
  }
  std::uint32_t get_remote_qp_num() const noexcept { return remote_port_; }
  asio::ip::address get_local_address() const noexcept {
    return local_address_;
  }
  std::uint32_t get_local_qp_num() const noexcept { return local_port_; }

  // ---- connection lifecycle (used by nd_io free functions) ----------------

  async_simple::coro::Lazy<std::error_code> connect(
      const std::string& host, const std::string& port) noexcept {
    try {
      auto address = asio::ip::make_address(host);
      tcp::endpoint endpoint(address,
                             static_cast<asio::ip::port_type>(std::stoi(port)));
      auto ec = co_await connect_one_endpoint(endpoint);
      if (ec) [[unlikely]] {
        close();
        co_return ec;
      }
      co_return std::error_code{};
    } catch (const std::exception& e) {
      ELOG_ERROR << "nd_socket connect failed: " << e.what();
      close();
      co_return std::make_error_code(std::errc::protocol_error);
    }
  }

  template <typename EndPoint>
  async_simple::coro::Lazy<std::error_code> connect(
      const EndPoint& endpoint) noexcept {
    try {
      if constexpr (requires {
                      endpoint.address();
                      endpoint.port();
                    }) {
        auto ec = co_await connect_one_endpoint(endpoint);
        if (ec) [[unlikely]] {
          close();
        }
        co_return ec;
      }
      else {
        std::error_code last_ec =
            std::make_error_code(std::errc::invalid_argument);
        bool tried = false;
        for (auto const& ep : endpoint) {
          tried = true;
          auto ec = co_await connect_one_endpoint(ep);
          if (!ec) [[likely]] {
            co_return std::error_code{};
          }
          last_ec = ec;
          close();
          init(conf_);
        }
        close();
        co_return tried ? last_ec
                        : std::make_error_code(std::errc::invalid_argument);
      }
    } catch (const std::exception& e) {
      ELOG_ERROR << "nd_socket connect failed: " << e.what();
      close();
      co_return std::make_error_code(std::errc::protocol_error);
    }
  }

  // Adopt a connector handed over by the listener, then accept on it.
  async_simple::coro::Lazy<std::error_code> accept(
      nd_connector<tcp>&& conn,
      std::span<const std::byte> peer_private_data = {}) noexcept {
    if (!apply_peer_info(peer_private_data)) [[unlikely]] {
      close();
      co_return std::make_error_code(std::errc::protocol_error);
    }
    auto self = state_;
    self->conn_.emplace(std::move(conn));
    auto local_info = make_socket_info();
    coro_io::callback_awaitor<std::error_code> awaitor;
    auto ec = co_await awaitor.await_resume([&](auto handler) {
      self->conn_->async_accept(
          self->qp_, asio::buffer(&local_info, sizeof(local_info)),
          [handler](asio::error_code ec) mutable {
            handler.set_value_then_resume(std::error_code(ec));
          });
    });
    if (ec) [[unlikely]] {
      close();
      co_return ec;
    }
    self->arm_recv_buffers();
    co_return std::error_code{};
  }

  // ---- data plane (used by nd_io free functions) --------------------------

  // Send one already-registered buffer view; completes with (ec, bytes).
  async_simple::coro::Lazy<std::pair<std::error_code, std::size_t>>
  async_send_one(const_buffer view) {
    auto self = state_;
    if (!self) [[unlikely]] {
      co_return std::pair{std::make_error_code(std::errc::bad_file_descriptor),
                          std::size_t{0}};
    }
    coro_io::callback_awaitor<std::pair<std::error_code, std::size_t>> awaitor;
    auto result = co_await awaitor.await_resume([&](auto handler) {
      self->qp_.async_send(
          view, [handler](asio::error_code ec, std::size_t n) mutable {
            handler.set_value_then_resume(std::pair{std::error_code(ec), n});
          });
    });
    co_return result;
  }

  // Wait for the next completed recv; on success the data is in the current
  // recv buffer (consume() copies it out). Returns (ec, len).
  async_simple::coro::Lazy<std::pair<std::error_code, std::size_t>>
  async_recv_one() {
    auto self = state_;
    if (!self) [[unlikely]] {
      co_return std::pair{std::make_error_code(std::errc::bad_file_descriptor),
                          std::size_t{0}};
    }
    coro_io::callback_awaitor<std::pair<std::error_code, std::size_t>> awaitor;
    auto result = co_await awaitor.await_resume([&](auto handler) {
      self->post_recv_impl(
          [handler](std::pair<std::error_code, std::size_t> r) mutable {
            handler.set_value_then_resume(std::move(r));
          });
    });
    co_return result;
  }

  std::size_t remain_read_buffer_size() const noexcept {
    return state_ ? state_->remain_read_buffer_size() : 0;
  }
  std::size_t consume(char* dst, std::size_t sz) {
    return state_ ? state_->consume(dst, sz) : 0;
  }

  void close() {
    if (state_) {
      state_->close();
    }
  }
  void cancel() const {
    if (state_) {
      auto self = state_;
      asio::dispatch(executor_->get_asio_executor(), [self]() {
        self->cancel();
      });
    }
  }

 private:
  async_simple::coro::Lazy<std::error_code> connect_one_endpoint(
      const tcp::endpoint& endpoint) {
    auto self = state_;
    set_remote_endpoint(endpoint);
    set_local_address_by_family(endpoint.address().is_v4());
    self->conn_.emplace(self->io_ctx_);
    self->conn_->open(endpoint.address().is_v4() ? tcp::v4() : tcp::v6());
    auto ec = co_await connect_op(endpoint);
    if (ec) [[unlikely]] {
      co_return ec;
    }
    self->arm_recv_buffers();
    co_return std::error_code{};
  }

  template <typename EndPoint>
  async_simple::coro::Lazy<std::error_code> connect_op(
      const EndPoint& endpoint) {
    auto self = state_;
    auto local_info = make_socket_info();
    nd_socket_info peer_info{};
    coro_io::callback_awaitor<std::error_code> awaitor;
    auto ec = co_await awaitor.await_resume([&](auto handler) {
      self->conn_->async_connect(
          self->qp_, endpoint, asio::buffer(&local_info, sizeof(local_info)),
          asio::buffer(&peer_info, sizeof(peer_info)),
          [handler](asio::error_code ec, std::size_t) mutable {
            handler.set_value_then_resume(std::error_code(ec));
          });
    });
    if (!ec && !apply_peer_info(peer_info)) [[unlikely]] {
      co_return std::make_error_code(std::errc::protocol_error);
    }
    co_return ec;
  }

  nd_socket_info make_socket_info() const noexcept {
    nd_socket_info info;
    info.magic = nd_socket_info_magic;
    info.buffer_size = static_cast<std::uint32_t>(
        (std::min<std::size_t>)(buffer_size_, UINT32_MAX));
    info.port = local_port_;
    if (!local_address_.is_unspecified()) {
      if (local_address_.is_v4()) {
        auto bytes = local_address_.to_v4().to_bytes();
        info.address_size = static_cast<std::uint16_t>(bytes.size());
        std::copy(bytes.begin(), bytes.end(), info.address.begin());
      }
      else {
        auto bytes = local_address_.to_v6().to_bytes();
        info.address_size = static_cast<std::uint16_t>(bytes.size());
        std::copy(bytes.begin(), bytes.end(), info.address.begin());
      }
    }
    return info;
  }

  bool apply_peer_info(const nd_socket_info& info) noexcept {
    if (info.magic != nd_socket_info_magic || info.buffer_size == 0) {
      return false;
    }
    auto pool = buffer_pool();
    if (!pool) [[unlikely]] {
      return false;
    }
    buffer_size_ =
        (std::min<std::size_t>)(pool->buffer_size(), info.buffer_size);
    remote_port_ = info.port;
    try {
      if (info.address_size == 4) {
        std::array<unsigned char, 4> bytes{};
        std::copy_n(info.address.begin(), bytes.size(), bytes.begin());
        remote_address_ = asio::ip::make_address_v4(bytes);
      }
      else if (info.address_size == 16) {
        std::array<unsigned char, 16> bytes{};
        std::copy_n(info.address.begin(), bytes.size(), bytes.begin());
        remote_address_ = asio::ip::make_address_v6(bytes);
      }
    } catch (...) {
      return false;
    }
    return true;
  }

  bool apply_peer_info(std::span<const std::byte> data) noexcept {
    if (data.size() < sizeof(nd_socket_info)) {
      return false;
    }
    nd_socket_info info;
    std::memcpy(&info, data.data(), sizeof(info));
    return apply_peer_info(info);
  }

  void set_remote_endpoint(const tcp::endpoint& endpoint) noexcept {
    remote_address_ = endpoint.address();
    remote_port_ = endpoint.port();
  }

  void set_local_address_by_family(bool use_v4) {
    local_address_ = use_v4 ? conf_.device->get_v4_address()
                            : conf_.device->get_v6_address();
  }

  void init(const config_t& config) {
    conf_ = config;
    if (conf_.device == nullptr) {
      conf_.device =
          nd_device_manager_t::instance().get_first_available_device({});
    }
    conf_.recv_buffer_cnt = (std::max<std::uint16_t>)(conf_.recv_buffer_cnt, 1);
    conf_.send_buffer_cnt = (std::max<std::uint16_t>)(conf_.send_buffer_cnt, 1);
    buffer_size_ = conf_.buffer_size;
    if (conf_.device) {
      try {
        local_address_ = conf_.device->get_v4_address();
      } catch (...) {
        try {
          local_address_ = conf_.device->get_v6_address();
        } catch (...) {
          local_address_ = {};
        }
      }
    }

    auto pool_config = conf_.buffer_pool_config;
    pool_config.buffer_size = conf_.buffer_size;
    auto pool = nd_buffer_pool_t::create(conf_.device, pool_config);
    state_ = std::make_shared<detail::nd_socket_shared_state_t>(
        executor_, conf_.device, std::move(pool), conf_.recv_buffer_cnt);
  }

  coro_io::ExecutorWrapper<>* executor_;
  config_t conf_;
  std::size_t buffer_size_ = 0;
  asio::ip::address local_address_;
  asio::ip::address remote_address_;
  std::uint16_t local_port_ = 0;
  std::uint16_t remote_port_ = 0;
  std::shared_ptr<detail::nd_socket_shared_state_t> state_;
};

}  // namespace coro_io
