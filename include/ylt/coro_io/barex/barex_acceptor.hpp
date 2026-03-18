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
#include "ylt/easylog.hpp"
#define CMAKE_INCLUDE
#include <accl/barex/barex_types.h>
#include <accl/barex/xcontext.h>
#include <accl/barex/xlistener.h>

#include <cstdint>
#include <memory>
#include <system_error>

#include "async_simple/coro/ConditionVariable.h"
#include "async_simple/coro/Lazy.h"
#include "async_simple/coro/Mutex.h"
#include "ylt/coro_io/io_context_pool.hpp"
#include "ylt/util/concurrentqueue.h"
namespace coro_io {
struct barex_context_t;
struct barex_socket_impl_t;
struct barex_acceptor_impl_t
    : public std::enable_shared_from_this<barex_acceptor_impl_t> {
  coro_io::ExecutorWrapper<>* executor_;
  async_simple::coro::ConditionVariable<async_simple::coro::Mutex> cv_;
  async_simple::coro::Mutex mutex_;
  std::atomic<std::size_t> new_connection_cnt_ = 0;
  std::unique_ptr<accl::barex::XListener> listener_;
  std::vector<std::shared_ptr<barex_context_t>> ctxs_;
  std::atomic<bool> has_closed_ = false;
  uint16_t port_;
  ylt::detail::moodycamel::ConcurrentQueue<std::shared_ptr<barex_socket_impl_t>>
      sockets;
  barex_acceptor_impl_t(
      coro_io::ExecutorWrapper<>* executor, uint16_t port,
      std::vector<std::shared_ptr<coro_io::barex_context_t>> ctxs)
      : executor_(executor), port_(port), ctxs_(std::move(ctxs)) {}
  std::error_code listen();
  async_simple::coro::Lazy<std::shared_ptr<barex_socket_impl_t>> accept();
  async_simple::coro::Lazy<std::shared_ptr<barex_socket_impl_t>> accept_impl();
  void accept_connection(std::shared_ptr<barex_socket_impl_t> soc) {
    if (sockets.try_enqueue(std::move(soc))) {
      ++new_connection_cnt_;
      ELOG_TRACE << "port: " << port_
                 << " notify acceptor, connection cnt: " << new_connection_cnt_;
      cv_.notify();
    }
    else {
      ELOG_ERROR << "port: " << port_
                 << " accept_connection failed: enqueue socket failed, "
                    "connection dropped";
    }
  }
  void close() {
    bool expected = false;
    if (has_closed_.compare_exchange_strong(expected, true)) {
      ELOG_DEBUG << "acceptor closed";
      accl::barex::Status result = listener_->Shutdown();
      if (!result.IsOk()) {
        ELOG_ERROR << "barex listener shutdown failed: " << result.ErrMsg();
      }
      try {
        result = listener_->WaitStop();

        if (!result.IsOk()) {
          ELOG_ERROR << "barex listener WaitStop failed: " << result.ErrMsg();
        }
      } catch (const std::exception& e) {
        ELOG_ERROR << "barex listener WaitStop failed: " << e.what();
      } catch (...) {
        ELOG_ERROR << "barex listener WaitStop failed";
      }
      cv_.notify();
    }
  }
  ~barex_acceptor_impl_t() { close(); }
};

using barex_acceptor_t = std::shared_ptr<barex_acceptor_impl_t>;

}  // namespace coro_io