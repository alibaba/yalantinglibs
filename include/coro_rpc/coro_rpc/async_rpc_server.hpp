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
#include <cassert>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <future>
#include <mutex>
#include <unordered_map>

#include "asio_util/io_context_pool.hpp"
#include "async_connection.hpp"
#include "common_service.hpp"
#include "coro_rpc/coro_rpc/coro_rpc_server.hpp"
#include "coro_rpc/coro_rpc/rpc_protocol.h"
#include "remote.hpp"
#include "util/utils.hpp"

namespace coro_rpc {
/*!
 * TODO: add doc
 */
class async_rpc_server {
  enum class stat {
    init = 0,  // server hasn't started/stopped.
    started,   // server is started
    stop,      // server is stopping/stopped
  };

 public:
  /*!
   * Construct a async server
   *
   * @param thread_num
   * @param port
   * @param conn_timeout_duration
   */
  async_rpc_server(size_t thread_num, unsigned short port,
                   std::chrono::steady_clock::duration conn_timeout_duration =
                       std::chrono::seconds(0))
      : pool_(thread_num),
        acceptor_(pool_.get_io_context()),
        port_(port),
        conn_timeout_duration_(conn_timeout_duration),
        signals_(pool_.get_io_context()),
        flag_(stat::init) {
    signals_.add(SIGINT);
    signals_.add(SIGTERM);
#if defined(SIGQUIT)
    signals_.add(SIGQUIT);
#endif  // defined(SIGQUIT)
    signals_.async_wait([this](std::error_code /*ec*/, int /*signo*/) {
      stop();
    });
  }
#ifdef ENABLE_SSL
  void init_ssl_context(const ssl_configure &conf) {
    use_ssl_ = init_ssl_context_helper(context_, conf);
  }
#endif

  [[nodiscard]] std::errc start() {
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

      ec = init_acceptor();
      if (ec == err_ok) {
        do_accept();
        flag_ = stat::started;
      }
      else {
        flag_ = stat::stop;
      }
    }

    cond_.notify_all();
    if (ec == err_ok) {
      pool_.run();
    }
    return ec;
  }

  [[nodiscard]] bool wait_for_start(auto duration) {
    std::unique_lock lock(start_mtx_);
    return cond_.wait_for(lock, duration, [this] {
      return flag_ == stat::started || flag_ == stat::stop;
    });
  }

  [[nodiscard]] std::errc async_start() {
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

      std::promise<std::errc> promise;
      auto future = promise.get_future();
      thd_ = std::thread([this, &ec, &promise] {
        ec = init_acceptor();
        if (ec == std::errc{}) {
          do_accept();
          promise.set_value(ec);
          pool_.run();
        }
        else {
          promise.set_value(ec);
        }
      });
      ec = future.get();
      flag_ = ec == err_ok ? stat::started : stat::stop;
    }

    cond_.notify_all();

    return ec;
  }

  void stop() {
    std::unique_lock lock(start_mtx_);

    if (flag_ == stat::stop) {
      if (thd_.joinable()) {
        thd_.join();
      }
      return;
    }
    easylog::info("begin to stop async_rpc_server");
    close_acceptor();
    if (flag_ == stat::started) {
      // notify connection quit all async event
      for (auto &pair : conns_) {
        if (auto conn = pair.second.lock(); conn) {
          conn->quit();
        }
      }
      pool_.stop();
    }
    if (thd_.joinable()) {
      thd_.join();
    }

    easylog::info("stop async_rpc_server ok");
    flag_ = stat::stop;
  }

  uint16_t port() const { return port_; };

  ~async_rpc_server() { stop(); }

 private:
  void do_accept() {
    auto conn = std::make_shared<async_connection>(pool_.get_io_context(),
                                                   conn_timeout_duration_);
#ifdef ENABLE_SSL
    if (use_ssl_) {
      conn->init_ssl(context_);
    }
#endif
    if (!promise_) [[unlikely]] {
      promise_ = std::promise<void>{};
    }
    acceptor_.async_accept(
        conn->socket(), [this, conn](asio::error_code ec) mutable {
          if (ec) {
            easylog::info("acceptor error: {}", ec.message());
            if (ec == asio::error::operation_aborted) {
              promise_.value().set_value();
              return;
            }
          }
          else {
            int64_t conn_id = ++conn_id_;
            easylog::info("new client conn_id {} coming", conn_id);
            conns_.emplace(conn_id, conn);
            conn->start();
          }

          do_accept();
        });
  }

  std::errc init_acceptor() {
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
    return std::errc{};
  }

  void close_acceptor() {
    if (!acceptor_.is_open()) {
      return;
    }

    asio::post(acceptor_.get_executor(), [this]() {
      asio::error_code ec;
      acceptor_.cancel(ec);
      acceptor_.close(ec);
    });

    if (promise_) {
      promise_.value().get_future().wait();
    }
  }

  io_context_pool pool_;
  tcp::acceptor acceptor_;
  std::atomic<uint16_t> port_;
  std::chrono::steady_clock::duration conn_timeout_duration_;
  std::thread thd_;
  asio::signal_set signals_;
  std::optional<std::promise<void>> promise_;
  stat flag_;

  std::mutex start_mtx_;
  std::condition_variable cond_;

  uint64_t conn_id_ = 0;
  std::unordered_map<uint64_t, std::weak_ptr<async_connection>> conns_;
#ifdef ENABLE_SSL
  asio::ssl::context context_{asio::ssl::context::sslv23};
  bool use_ssl_ = false;
#endif
};
}  // namespace coro_rpc