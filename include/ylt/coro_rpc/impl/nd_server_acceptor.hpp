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

#ifdef YLT_ENABLE_ND
#include <algorithm>
#include <atomic>
#include <cassert>
#include <cstdint>
#include <exception>
#include <future>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>
#include <ylt/easylog.hpp>

#include "asio/dispatch.hpp"
#include "asio/error_code.hpp"
#include "asio/io_context.hpp"
#include "asio/socket_base.hpp"
#include "async_simple/coro/Lazy.h"
#include "ylt/coro_io/networkdirect/nd_io.hpp"
#include "ylt/coro_io/networkdirect/nd_use_device.hpp"
#include "ylt/coro_io/server_acceptor.hpp"

namespace coro_rpc {

class nd_server_acceptor : public coro_io::server_acceptor_base {
 public:
  nd_server_acceptor(std::string_view address, uint16_t port,
                     const coro_io::nd_socket_t::config_t& conf,
                     const coro_io::server_acceptor_base* fallback_acceptor =
                         nullptr)
      : server_acceptor_base(address, port),
        config_(conf),
        fallback_acceptor_(fallback_acceptor) {}

  coro_io::listen_errc listen() override {
    executor_ = pool_->get_executor();

    auto fallback_base_port =
        fallback_acceptor_ == nullptr ? 0 : fallback_acceptor_->port();
    if (port_ == 0 && fallback_base_port != 0) {
      if (fallback_base_port == std::numeric_limits<uint16_t>::max()) {
        ELOG_ERROR << "NetworkDirect listen failed: tcp port "
                   << fallback_base_port
                   << " cannot be converted to nd port + 1";
        return coro_io::listen_errc::bad_address;
      }
      port_ = static_cast<uint16_t>(fallback_base_port + 1);
    }
    if (port_ == 0) {
      ELOG_ERROR << "NetworkDirect listen failed: nd_port is 0 and no fixed "
                    "tcp port is available for tcp_port + 1";
      return coro_io::listen_errc::bad_address;
    }

    asio::error_code ec;
    if (!init_context(executor_, ec)) {
      ELOG_ERROR << "NetworkDirect use_device failed: " << ec.message();
      return coro_io::listen_errc::open_error;
    }

    // nd_address selects v4/v6 and validates the requested family. The actual
    // local ND address comes from config_.device.
    auto endpoint = coro_io::detail::resolve_listen_endpoint(
        executor_->get_asio_executor(), address_, port_, ec);
    if (!endpoint) {
      ELOG_ERROR << "resolve NetworkDirect address " << address_
                 << " error: " << ec.message();
      return coro_io::listen_errc::bad_address;
    }

    use_v4_ = endpoint->address().is_v4();
    try {
      if (use_v4_) {
        (void)config_.device->get_v4_address();
      }
      else {
        (void)config_.device->get_v6_address();
      }
    } catch (const std::exception& e) {
      ELOG_ERROR << "NetworkDirect device address family mismatch: "
                 << e.what();
      return coro_io::listen_errc::bad_address;
    }

    acceptor_.emplace(static_cast<asio::io_context&>(executor_->context()));
    acceptor_->open(use_v4_ ? coro_io::tcp::v4() : coro_io::tcp::v6(), ec);
    if (ec) {
      ELOG_ERROR << "NetworkDirect open failed: " << ec.message();
      return coro_io::listen_errc::open_error;
    }
    acceptor_->bind(port_, ec);
    if (ec) {
      ELOG_ERROR << "NetworkDirect bind port " << port_
                 << " error: " << ec.message();
      acceptor_->cancel();
      return coro_io::listen_errc::address_in_used;
    }
    acceptor_->listen(asio::socket_base::max_listen_connections, ec);
    if (ec) {
      ELOG_ERROR << "NetworkDirect listen port " << port_
                 << " error: " << ec.message();
      acceptor_->cancel();
      return coro_io::listen_errc::listen_error;
    }

    ELOG_INFO << "NetworkDirect listen port " << port_ << " successfully";
    return coro_io::listen_errc::ok;
  }

  async_simple::coro::Lazy<
      ylt::expected<coro_io::socket_wrapper_t, std::error_code>>
  accept() override {
    accept_started_.store(true, std::memory_order_release);
    assert(acceptor_.has_value());

    auto socket_executor = pool_->get_executor();
    asio::error_code ec;
    // Accepted sockets may run on different executors, so ensure each executor
    // has the ND device registered before creating socket state on it.
    if (!init_context(socket_executor, ec)) {
      ELOG_ERROR << "NetworkDirect use_device failed: " << ec.message();
      notify_accept_finished();
      co_return ylt::expected<coro_io::socket_wrapper_t, std::error_code>{
          ylt::unexpected<std::error_code>{asio::error::bad_descriptor}};
    }

    coro_io::nd_socket_t socket(socket_executor, config_);
    set_local_endpoint(socket);
    ELOG_TRACE << "start accepting from NetworkDirect acceptor: " << address_
               << ":" << port_;
    co_await coro_io::dispatch(executor_->get_asio_executor());
    auto error = co_await coro_io::async_accept(*acceptor_, socket);
    ELOG_TRACE << "get connection from NetworkDirect acceptor: " << address_
               << ":" << port_;
    if (error) {
      if (error == asio::error::operation_aborted) {
        ELOG_INFO << "NetworkDirect accept canceled by server quit: "
                  << error.message();
      }
      else {
        ELOG_ERROR << "NetworkDirect accept failed: " << error.message();
      }
      if (error == asio::error::operation_aborted ||
          error == asio::error::bad_descriptor) {
        notify_accept_finished();
      }
      co_return ylt::expected<coro_io::socket_wrapper_t, std::error_code>{
          ylt::unexpected<std::error_code>{error}};
    }

    co_return coro_io::socket_wrapper_t{std::move(socket), socket_executor};
  }

  void close_now() override {
    if (!acceptor_) {
      return;
    }
    if (!accept_started_.load(std::memory_order_acquire)) {
      acceptor_->cancel();
      return;
    }
    asio::dispatch(executor_->get_asio_executor(), [this]() {
      if (acceptor_) {
        acceptor_->cancel();
      }
    });
  }

  void close() override {
    close_now();
    if (accept_started_.load(std::memory_order_acquire)) {
      acceptor_close_future_.wait();
    }
    stop_contexts();
  }

 private:
  bool prepare_config() {
    if (!config_.device) {
      config_.device =
          coro_io::nd_device_manager_t::instance().get_first_available_device(
              {});
    }
    if (!config_.device) {
      ELOG_ERROR << "NetworkDirect listen failed: no available device";
      return false;
    }
    return true;
  }

  bool init_context(coro_io::ExecutorWrapper<>* executor,
                    asio::error_code& ec) {
    if (!prepare_config()) {
      ec = coro_io::make_error_code(coro_io::rdma_errc::no_available_device);
      return false;
    }
    coro_io::nd_config_t config{};
    config.cqe_ = config_.cq_size;
    coro_io::use_device(static_cast<asio::io_context&>(executor->context()),
                        config_.device, config, ec);
    if (ec == coro_io::make_error_code(
                  coro_io::rdma_errc::already_registered)) {
      ec.clear();
    }
    if (!ec) {
      remember_executor(executor);
    }
    return !ec;
  }

  void remember_executor(coro_io::ExecutorWrapper<>* executor) {
    std::lock_guard lock(executors_mtx_);
    if (std::find(executors_.begin(), executors_.end(), executor) ==
        executors_.end()) {
      executors_.push_back(executor);
    }
  }

  void stop_contexts() {
    std::lock_guard lock(executors_mtx_);
    for (auto* executor : executors_) {
      if (executor != nullptr) {
        static_cast<asio::io_context&>(executor->context()).stop();
      }
    }
  }

  void notify_accept_finished() noexcept {
    if (accept_finished_.exchange(true, std::memory_order_acq_rel)) {
      return;
    }
    try {
      acceptor_close_waiter_.set_value();
    } catch (...) {
    }
  }

  void set_local_endpoint(coro_io::nd_socket_t& socket) {
    try {
      auto address = use_v4_ ? config_.device->get_v4_address()
                             : config_.device->get_v6_address();
      socket.set_local_endpoint(std::move(address), port_);
    } catch (const std::exception& e) {
      ELOG_WARN << "set NetworkDirect local endpoint failed: " << e.what();
    }
  }

  coro_io::nd_socket_t::config_t config_;
  const coro_io::server_acceptor_base* fallback_acceptor_ = nullptr;
  bool use_v4_ = true;
  std::optional<coro_io::nd_listener<coro_io::tcp>> acceptor_;
  coro_io::ExecutorWrapper<>* executor_ = nullptr;
  std::mutex executors_mtx_;
  std::vector<coro_io::ExecutorWrapper<>*> executors_;
  std::promise<void> acceptor_close_waiter_;
  std::future<void> acceptor_close_future_ =
      acceptor_close_waiter_.get_future();
  std::atomic<bool> accept_started_ = false;
  std::atomic<bool> accept_finished_ = false;
};

}  // namespace coro_rpc
#endif
