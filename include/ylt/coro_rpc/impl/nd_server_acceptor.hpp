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
#include <chrono>
#include <cstdint>
#include <exception>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <ylt/easylog.hpp>

#include "asio/error_code.hpp"
#include "asio/io_context.hpp"
#include "asio/ip/tcp.hpp"
#include "asio/socket_base.hpp"
#include "async_simple/coro/Lazy.h"
#include "async_simple/util/move_only_function.h"
#include "coro_connection.hpp"
#include "errno.h"
#include "ylt/coro_io/networkdirect/nd_io.hpp"
#include "ylt/coro_io/networkdirect/nd_use_device.hpp"

namespace coro_rpc {

template <typename server_config>
class nd_server_acceptor {
  using connection_transfer_t = async_simple::util::move_only_function<void(
      coro_io::socket_wrapper_t &&socket, std::string_view magic_number)>;

 public:
  void init(const coro_io::nd_socket_t::config_t &conf = {},
            uint16_t nd_port = 0, std::string nd_address = {}) {
    config_ = conf;
    port_ = nd_port;
    address_ = std::move(nd_address);
  }

  bool enabled() const noexcept { return config_.has_value(); }

  coro_rpc::err_code listen(typename server_config::executor_pool_t &pool,
                            const std::string &server_address,
                            uint16_t server_port) {
    if (!enabled()) {
      return {};
    }

    auto executor = pool.get_executor();
    asio::error_code ec;
    if (!init_context(executor, ec)) {
      ELOG_ERROR << "NetworkDirect use_device failed: " << ec.message();
      return coro_rpc::errc::open_error;
    }

    auto listen_address = address_.empty() ? server_address : address_;
    effective_port_ =
        port_ != 0 ? port_ : static_cast<uint16_t>(server_port + 1);

    asio::ip::tcp::resolver::query query(
        listen_address, std::to_string(effective_port_));
    asio::ip::tcp::resolver resolver(executor->get_asio_executor());
    auto it = resolver.resolve(query, ec);
    asio::ip::tcp::resolver::iterator it_end;
    if (ec || it == it_end) {
      ELOG_ERROR << "resolve NetworkDirect address " << listen_address
                 << " error: " << ec.message();
      return coro_rpc::errc::bad_address;
    }

    auto endpoint = it->endpoint();
    use_v4_ = endpoint.address().is_v4();
    try {
      if (use_v4_) {
        (void)config_->device->get_v4_address();
      }
      else {
        (void)config_->device->get_v6_address();
      }
    } catch (const std::exception &e) {
      ELOG_ERROR << "NetworkDirect device address family mismatch: "
                 << e.what();
      return coro_rpc::errc::bad_address;
    }

    acceptor_.emplace(static_cast<asio::io_context &>(executor->context()));
    acceptor_->open(use_v4_ ? coro_io::tcp::v4() : coro_io::tcp::v6(), ec);
    if (ec) {
      ELOG_ERROR << "NetworkDirect open failed: " << ec.message();
      return coro_rpc::errc::open_error;
    }
    acceptor_->bind(effective_port_, ec);
    if (ec) {
      ELOG_ERROR << "NetworkDirect bind port " << effective_port_
                 << " error: " << ec.message();
      acceptor_->cancel();
      return coro_rpc::errc::address_in_used;
    }
    acceptor_->listen(asio::socket_base::max_listen_connections, ec);
    if (ec) {
      ELOG_ERROR << "NetworkDirect listen port " << effective_port_
                 << " error: " << ec.message();
      acceptor_->cancel();
      return coro_rpc::errc::listen_error;
    }

    ELOG_INFO << "NetworkDirect listen port " << effective_port_
              << " successfully";
    return {};
  }

  void start_accept(
      typename server_config::executor_pool_t &pool,
      std::chrono::steady_clock::duration conn_timeout_duration,
      typename server_config::rpc_protocol::router &router,
      std::atomic<uint64_t> &conn_id,
      std::unordered_map<uint64_t, std::shared_ptr<coro_connection>> &conns,
      std::mutex &conns_mtx, connection_transfer_t &connection_transfer)
      noexcept {
    if (!enabled() || !acceptor_) {
      return;
    }
    reset_accept_waiter();
    try {
      accept(pool, conn_timeout_duration, router, conn_id, conns, conns_mtx,
             connection_transfer)
          .start([this](auto &&res) mutable noexcept {
        try {
          if (res.hasError()) {
            ELOG_ERROR << "NetworkDirect accept loop failed";
          }
        } catch (...) {
        }
        notify_accept_finished();
      });
    } catch (const std::exception &e) {
      ELOG_ERROR << "NetworkDirect accept loop start failed: " << e.what();
      notify_accept_finished();
    } catch (...) {
      ELOG_ERROR << "NetworkDirect accept loop start failed";
      notify_accept_finished();
    }
  }

  void cancel() {
    if (acceptor_) {
      acceptor_->cancel();
    }
  }

  void wait_accept_finished() {
    std::shared_future<void> future;
    {
      std::lock_guard lock(accept_waiter_mtx_);
      if (!accept_started_ || accept_finished_) {
        return;
      }
      future = accept_close_future_;
    }
    if (future.valid()) {
      future.wait();
    }
  }

  void stop_contexts() {
    std::lock_guard lock(executors_mtx_);
    for (auto *executor : executors_) {
      if (executor != nullptr) {
        static_cast<asio::io_context &>(executor->context()).stop();
      }
    }
  }

 private:
  async_simple::coro::Lazy<coro_rpc::err_code> accept(
      typename server_config::executor_pool_t &pool,
      std::chrono::steady_clock::duration conn_timeout_duration,
      typename server_config::rpc_protocol::router &router,
      std::atomic<uint64_t> &conn_id,
      std::unordered_map<uint64_t, std::shared_ptr<coro_connection>> &conns,
      std::mutex &conns_mtx, connection_transfer_t &connection_transfer) {
    using rpc_protocol = typename server_config::rpc_protocol;

    for (;;) {
      auto executor = pool.get_executor();
      asio::error_code ec;
      if (!init_context(executor, ec)) {
        ELOG_ERROR << "NetworkDirect use_device failed: " << ec.message();
        co_return coro_rpc::errc::open_error;
      }

      coro_io::nd_socket_t socket(executor, *config_);
      set_local_endpoint(socket);
      auto error = co_await coro_io::async_accept(*acceptor_, socket);
      if (error) {
        if (error == asio::error::operation_aborted) {
          ELOG_INFO << "NetworkDirect server was canceled:" << error.message();
        }
        else {
          ELOG_ERROR << "NetworkDirect server accept failed:"
                     << error.message();
        }
        if (error == asio::error::operation_aborted ||
            error == asio::error::bad_descriptor) {
          co_return coro_rpc::errc::operation_canceled;
        }
        continue;
      }

      uint64_t id = ++conn_id;
      ELOG_INFO << "new NetworkDirect client conn_id " << id << " coming";
      coro_io::socket_wrapper_t wrapper{std::move(socket), executor};
      auto conn = std::make_shared<coro_connection>(std::move(wrapper),
                                                    conn_timeout_duration);
      conn->set_quit_callback(
          [&conns, &conns_mtx](const uint64_t &id) {
            std::unique_lock lock(conns_mtx);
            conns.erase(id);
          },
          id);
      if (connection_transfer) {
        conn->set_transfer_callback(&connection_transfer);
      }
      {
        std::unique_lock lock(conns_mtx);
        conns.emplace(id, conn);
      }
      conn->template start<rpc_protocol>(router)
          .via(conn->get_executor())
          .detach();
    }
  }

  void reset_accept_waiter() {
    std::lock_guard lock(accept_waiter_mtx_);
    accept_close_promise_ = std::make_shared<std::promise<void>>();
    accept_close_future_ = accept_close_promise_->get_future().share();
    accept_started_ = true;
    accept_finished_ = false;
  }

  void notify_accept_finished() noexcept {
    std::shared_ptr<std::promise<void>> promise;
    {
      std::lock_guard lock(accept_waiter_mtx_);
      if (!accept_started_ || accept_finished_) {
        return;
      }
      accept_finished_ = true;
      promise = accept_close_promise_;
    }
    try {
      if (promise) {
        promise->set_value();
      }
    } catch (...) {
    }
  }

  bool prepare_config() {
    if (!enabled()) {
      return false;
    }
    if (!config_->device) {
      config_->device =
          coro_io::nd_device_manager_t::instance().get_first_available_device(
              {});
    }
    if (!config_->device) {
      ELOG_ERROR << "NetworkDirect listen failed: no available device";
      return false;
    }
    return true;
  }

  bool init_context(coro_io::ExecutorWrapper<> *executor,
                    asio::error_code &ec) {
    if (!prepare_config()) {
      ec = coro_io::make_error_code(coro_io::rdma_errc::no_available_device);
      return false;
    }
    coro_io::nd_config_t config{};
    config.cqe_ = config_->cq_size;
    coro_io::use_device(static_cast<asio::io_context &>(executor->context()),
                        config_->device, config, ec);
    if (ec == coro_io::make_error_code(
                  coro_io::rdma_errc::already_registered)) {
      ec.clear();
    }
    if (!ec) {
      remember_executor(executor);
    }
    return !ec;
  }

  void remember_executor(coro_io::ExecutorWrapper<> *executor) {
    std::lock_guard lock(executors_mtx_);
    if (std::find(executors_.begin(), executors_.end(), executor) ==
        executors_.end()) {
      executors_.push_back(executor);
    }
  }

  void set_local_endpoint(coro_io::nd_socket_t &socket) {
    try {
      auto address = use_v4_ ? config_->device->get_v4_address()
                             : config_->device->get_v6_address();
      socket.set_local_endpoint(std::move(address), effective_port_);
    } catch (const std::exception &e) {
      ELOG_WARN << "set NetworkDirect local endpoint failed: " << e.what();
    }
  }

  std::optional<coro_io::nd_socket_t::config_t> config_;
  uint16_t port_ = 0;
  uint16_t effective_port_ = 0;
  std::string address_;
  bool use_v4_ = true;
  std::optional<coro_io::nd_listener<coro_io::tcp>> acceptor_;
  std::mutex executors_mtx_;
  std::vector<coro_io::ExecutorWrapper<> *> executors_;
  std::mutex accept_waiter_mtx_;
  std::shared_ptr<std::promise<void>> accept_close_promise_;
  std::shared_future<void> accept_close_future_;
  bool accept_started_ = false;
  bool accept_finished_ = true;
};

}  // namespace coro_rpc
#endif
