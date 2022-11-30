/*
 * Copyright (c) 2022, Alibaba Group Holding Limited;
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
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <future>
#include <memory>
#include <mutex>
#include <system_error>
#include <unordered_map>
#include <vector>

#include "asio/dispatch.hpp"
#include "asio/error_code.hpp"
#include "asio/io_context.hpp"
#include "asio_util/asio_coro_util.hpp"
#include "asio_util/asio_util.hpp"
#include "asio_util/io_context_pool.hpp"
#include "async_simple/coro/Lazy.h"
#include "common_service.hpp"
#include "coro_connection.hpp"
#include "coro_rpc/coro_rpc/rpc_protocol.h"
#include "logging/easylog.hpp"
#include "remote.hpp"
namespace coro_rpc {
/*!
 * ```cpp
 * #include <coro_rpc/coro_rpc_server.hpp>
 * inline std::string hello_coro_rpc() { return "hello coro_rpc"; }
 * int main(){
 *   register_handler<hello_coro_rpc>();
 *   coro_rpc_server server(std::thread::hardware_concurrency(), 9000);
 *   server.start();
 *   return 0;
 * }
 * ```
 */
class coro_rpc_server {
  //!< Server state
  enum class stat {
    init = 0,  // server hasn't started/stopped.
    started,   // server is started
    stop       // server is stopping/stopped
  };

 public:
  /*!
   * TODO: add doc
   * @param thread_num the number of io_context.
   * @param port the server port to listen.
   * @param conn_timeout_duration client connection timeout. 0 for no timeout.
   *                              default no timeout.
   */
  coro_rpc_server(size_t thread_num, unsigned short port,
                  std::chrono::steady_clock::duration conn_timeout_duration =
                      std::chrono::seconds(0))
      : pool_(thread_num),
        acceptor_(pool_.get_io_context()),
        port_(port),
        executor_(pool_.get_io_context()),
        conn_timeout_duration_(conn_timeout_duration),
        flag_{stat::init} {}

  ~coro_rpc_server() {
    // FIXME: when coro_rpc_server is global variable, spdlog's log level may
    // destructe first, which cause crash.
    // easylog::info("coro_rpc_server will quit");
    stop();
  }

#ifdef ENABLE_SSL
  void init_ssl_context(const ssl_configure &conf) {
    use_ssl_ = init_ssl_context_helper(context_, conf);
  }
#endif

  /*!
   * Start the server in blocking mode
   *
   * If the port is used, it will return a non-empty error code.
   *
   * @return error code if start failed, otherwise block until server stop.
   */
  [[nodiscard]] std::errc start() noexcept {
    std::errc ec{};
    {
      std::unique_lock lock(start_mtx_);
      if (flag_ != stat::init) {
        if (flag_ == stat::started) {
          easylog::info("start again");
        }
        else if (flag_ == stat::started) {
          easylog::info("has stoped");
        }
        return std::errc::io_error;
      }

      ec = listen();
      if (ec == err_ok) {
        thd_ = std::thread([this] {
          pool_.run();
        });
        acceptor_thd_ = std::thread([this] {
          acceptor_ioc_.run();
        });

        flag_ = stat::started;
      }
      else {
        flag_ = stat::stop;
      }
    }

    cond_.notify_all();

    if (ec == err_ok) {
      async_simple::coro::syncAwait(accept());
    }
    return ec;
  }

  [[nodiscard]] bool wait_for_start(auto duration) {
    std::unique_lock lock(start_mtx_);
    return cond_.wait_for(lock, duration, [this] {
      return flag_ == stat::started || flag_ == stat::stop;
    });
  }

  [[nodiscard]] async_simple::coro::Lazy<std::errc> async_start() noexcept {
    std::errc ec{};
    {
      std::unique_lock lock(start_mtx_);
      if (flag_ != stat::init) {
        if (flag_ == stat::started) {
          easylog::info("start again");
        }
        else if (flag_ == stat::started) {
          easylog::info("has stoped");
        }
        co_return std::errc::io_error;
      }

      ec = listen();
      if (ec == std::errc{}) {
        thd_ = std::thread([this] {
          pool_.run();
        });
        acceptor_thd_ = std::thread([this] {
          acceptor_ioc_.run();
        });
        flag_ = stat::started;
      }
      else {
        flag_ = stat::stop;
      }
    }

    cond_.notify_all();
    if (ec == err_ok) {
      co_await accept();
    }
    co_return ec;
  }

  /*!
   * Stop server
   *
   * Block and wait server stops.
   */
  void stop() {
    std::unique_lock lock(start_mtx_);
    if (flag_ == stat::stop) {
      return;
    }

    easylog::info("begin to stop coro_rpc_server, conn size {}", conns_.size());
    close_acceptor();
    if (flag_ == stat::started) {
      {
        std::unique_lock lock(mtx_);
        for (auto &conn : conns_) {
          conn.second->set_quit_callback(nullptr, 0);
          if (!conn.second->has_closed()) {
            conn.second->sync_close(false);
          }

          conn.second->wait_quit();
        }

        conns_.clear();
      }

      pool_.stop();
    }
    if (thd_.joinable()) {
      thd_.join();
    }

    if (acceptor_thd_.joinable()) {
      acceptor_ioc_.stop();
      acceptor_thd_.join();
    }
    easylog::info("stop coro_rpc_server ok");
    flag_ = stat::stop;
  }

  /*!
   * Get listening port
   * @return
   */
  uint16_t port() const { return port_; };

  /*!
   * Get inner executor
   * @return
   */
  auto &get_executor() { return executor_; }

 private:
  std::errc listen() {
    easylog::info("begin to listen");
    using asio::ip::tcp;
    auto endpoint = tcp::endpoint(tcp::v4(), port_);
    acceptor_.open(endpoint.protocol());
#ifdef __GNUC__
    acceptor_.set_option(tcp::acceptor::reuse_address(true));
#endif
    asio::error_code ec;
    acceptor_.bind(endpoint, ec);
    if (ec) {
      easylog::error("bind port {} error : {}", port_, ec.message());
      acceptor_.cancel(ec);
      acceptor_.close(ec);
      start_accept_promise_.set_value();
      close_accept_promise_.set_value();
      return std::errc::address_in_use;
    }
#ifdef _MSC_VER
    acceptor_.set_option(tcp::acceptor::reuse_address(true));
#endif
    acceptor_.listen();

    auto end_point = acceptor_.local_endpoint(ec);
    if (ec) {
      easylog::error("get local endpoint port {} error {}", port_,
                     ec.message());
      return std::errc::address_in_use;
    }
    port_ = end_point.port();

    easylog::info("listen port {} successfully", port_);
    start_accept_promise_.set_value();
    return {};
  }

  async_simple::coro::Lazy<std::errc> accept() {
    for (;;) {
      auto &io_context = pool_.get_io_context();
      asio::ip::tcp::socket socket(io_context);
      auto error = co_await asio_util::async_accept(acceptor_, socket);
#ifdef UNIT_TEST_INJECT
      if (g_action == inject_action::force_inject_server_accept_error) {
        asio::error_code ignored_ec;
        socket.shutdown(asio::ip::tcp::socket::shutdown_both, ignored_ec);
        socket.close(ignored_ec);
        error = make_error_code(std::errc::io_error);
        // only inject once
        g_action = inject_action::nothing;
      }
#endif
      if (error) {
        easylog::error("accept failed, error: {}", error.message());
        if (error == asio::error::operation_aborted) {
          close_accept_promise_.set_value();
          co_return std::errc::io_error;
        }
        continue;
      }

      int64_t conn_id = ++conn_id_;
      easylog::info("new client conn_id {} coming", conn_id);
      auto conn = std::make_shared<coro_connection>(
          io_context, std::move(socket), conn_timeout_duration_);
      conn->set_quit_callback(
          [this](const uint64_t &id) {
            std::unique_lock lock(mtx_);
            conns_.erase(id);
          },
          conn_id);

      {
        std::unique_lock lock(mtx_);
        conns_.emplace(conn_id, conn);
      }

      start_one(conn).via(&executor_).detach();
    }
  }

  async_simple::coro::Lazy<void> start_one(auto conn) noexcept {
#ifdef ENABLE_SSL
    if (use_ssl_) {
      conn->init_ssl(context_);
    }
#endif
    co_await conn->start();
  }

  void close_acceptor() {
    start_accept_promise_.get_future().wait();
    asio::dispatch(acceptor_.get_executor(), [this]() {
      asio::error_code ec;
      acceptor_.cancel(ec);
      acceptor_.close(ec);
    });
    close_accept_promise_.get_future().wait();
  }

  io_context_pool pool_;
  asio::io_context acceptor_ioc_;
  asio::ip::tcp::acceptor acceptor_;
  std::atomic<uint16_t> port_;
  asio_util::AsioExecutor executor_;
  std::chrono::steady_clock::duration conn_timeout_duration_;

  std::thread thd_;
  std::thread acceptor_thd_;
  std::promise<void> close_accept_promise_;
  std::promise<void> start_accept_promise_;
  stat flag_;

  std::mutex start_mtx_;
  std::condition_variable cond_;

  uint64_t conn_id_ = 0;
  std::unordered_map<uint64_t, std::shared_ptr<coro_connection>> conns_;
  std::mutex mtx_;
#ifdef ENABLE_SSL
  asio::ssl::context context_{asio::ssl::context::sslv23};
  bool use_ssl_ = false;
#endif
};
}  // namespace coro_rpc
