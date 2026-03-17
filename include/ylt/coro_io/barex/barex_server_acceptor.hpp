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
#include <accl/barex/barex_types.h>

#include <cstdint>
#include <memory>
#include <ylt/coro_io/server_acceptor.hpp>

#include "asio/error.hpp"
#include "asio/error_code.hpp"
#include "ylt/coro_io/barex/barex_acceptor.hpp"
#include "ylt/coro_io/barex/barex_device.hpp"
#include "ylt/coro_io/barex/barex_socket.hpp"
namespace coro_io {
struct barex_server_acceptor : public server_acceptor_base {
  barex_server_acceptor(uint16_t port) : server_acceptor_base("", port) {}
  barex_server_acceptor(uint16_t port,
                        const coro_io::barex_socket_t::config_t& config)
      : server_acceptor_base("", port), config_(config) {}
  virtual void close() override {
    if (accepter) {
      accepter->close();
    };
  }
  virtual listen_errc listen() override {
    std::vector<std::shared_ptr<coro_io::barex_context_t>> ctxs;
    auto executors = pool_->get_all_executor();
    ctxs.reserve(executors.size());
    if (config_.dev == nullptr) {
      config_.dev = coro_io::get_global_barex_device();
    }
    for (auto& e : executors) {
      ctxs.push_back(coro_io::get_barex_context(e.get(), config_.dev));
    }
    accepter = std::make_shared<coro_io::barex_acceptor_impl_t>(
        pool_->get_executor(), port_, std::move(ctxs));
    auto err = accepter->listen();
    if (err) {
      if (err.value() == accl::barex::BAREX_ERR_TCP) {
        return listen_errc::listen_error;
      }
      else {
        return listen_errc::unknown_error;
      }
    }
    else {
      return listen_errc::ok;
    }
  }
  virtual async_simple::coro::Lazy<
      ylt::expected<coro_io::socket_wrapper_t, std::error_code>>
  accept() override {
    ELOG_TRACE << "start accepting from barex acceptor: " << port_;
    auto socket_impl = co_await accepter->accept();
    ELOG_TRACE << "get connection from barex acceptor: " << port_;
    if SP_UNLIKELY (!socket_impl) {
      ELOG_INFO << "barex accept over, operation aborted.";
      co_return ylt::unexpected<std::error_code>{
          asio::error_code{asio::error::operation_aborted}};
    }
    else {
      barex_socket_t socket = {std::move(socket_impl), config_};
      co_return coro_io::socket_wrapper_t{std::move(socket)};
    }
  };
  barex_acceptor_t accepter = nullptr;
  coro_io::barex_socket_t::config_t config_;
};
}  // namespace coro_io