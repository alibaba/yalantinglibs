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

#include <chrono>
#include <mutex>
#include <system_error>
#include <thread>
#define CMAKE_INCLUDE
#include <accl/barex/barex.h>
#include <accl/barex/barex_types.h>
#include <accl/barex/xchannel.h>
#include <accl/barex/xconfig_util.h>
#include <accl/barex/xconnector.h>
#include <accl/barex/xcontext.h>
#include <accl/barex/xlistener.h>
#include <accl/barex/xsimple_mempool.h>
#include <accl/barex/xthreadpool.h>
#include <accl/barex/xtimer.h>
#include <async_simple/coro/Lazy.h>

#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>

#include "asio/posix/stream_descriptor.hpp"
#include "barex_acceptor.hpp"
#include "barex_device.hpp"
#include "ylt/coro_io/coro_io.hpp"
#include "ylt/coro_io/io_context_pool.hpp"
#include "ylt/easylog.hpp"
namespace coro_io {

namespace detail {
class barex_category : public std::error_category {
 public:
  barex_category() : error_message() {}
  const char* name() const noexcept override { return "barex error"; }
  std::string message(int ev) const override {
    std::stringstream s;

    s << (accl::barex::BarexResult)ev;
    return s.str();
  }
  std::string error_message;
};
inline std::error_code to_std_error_code(accl::barex::Status s) {
  static barex_category category;
  return std::error_code{s.ErrCode(), category};
}
}  // namespace detail

struct barex_socket_impl_t;

class barex_recv_callback_t : public accl::barex::XChannelCallback {
 public:
  void OnRecvCall(accl::barex::XChannel* channel, char* buf, size_t len,
                  accl::barex::x_msg_header header);
};
struct barex_context_t : std::enable_shared_from_this<barex_context_t> {
  std::shared_ptr<barex_device_t> dev_;
  ExecutorWrapper<>* executor_;
  std::unique_ptr<accl::barex::XContext> context_ = nullptr;
  std::unique_ptr<asio::posix::stream_descriptor> fd_ = nullptr;
  std::weak_ptr<barex_acceptor_impl_t> acceptor_;
  std::once_flag flag;

  barex_context_t(std::shared_ptr<barex_device_t> dev,
                  ExecutorWrapper<>* executor)
      : dev_(std::move(dev)), executor_(executor) {
    init();
  }

  static std::shared_ptr<barex_context_t> get_static_barex_context_impl(
      std::unordered_map<int, std::shared_ptr<barex_context_t>>& barex_ctx_map,
      std::shared_ptr<barex_device_t> dev,
      coro_io::ExecutorWrapper<>* executor) {
    auto [iter, is_new_ctx] =
        barex_ctx_map.try_emplace(dev->device_->GetId(), nullptr);
    if (is_new_ctx) [[unlikely]] {
      assert(iter->second == nullptr);
      iter->second =
          std::make_shared<barex_context_t>(std::move(dev), executor);
      iter->second->run();
    }
    return iter->second;
  }
  void close() {
    std::call_once(flag, [this]() {
      ELOG_DEBUG << "closing barex context";
      if (fd_) {
        std::error_code ec;
        [[maybe_unused]] auto _ = fd_->cancel(ec);
        fd_->release();
      }
      if (auto acceptor = acceptor_.lock()) {
        acceptor->close();
      }
    });
  }

  ~barex_context_t() { close(); }

 private:
  friend struct barex_acceptor_impl_t;
  accl::barex::BarexResult set_channel_callback();
  accl::barex::BarexResult init() {
    accl::barex::XContext* context;
    auto result = accl::barex::XContext::NewInstance(
        context, accl::barex::XConfigUtil::DefaultContextConfig(),
        new barex_recv_callback_t{}, dev_->device_, dev_->mempool_.get());
    if (result) {
      ELOG_WARN << "init XContext failed:" << result;
      return result;
    }
    assert(context != nullptr);
    context_.reset(context);
    result = set_channel_callback();
    return result;
  }
  void set_acceptor(barex_acceptor_impl_t* acceptor) {
    acceptor_ = acceptor->weak_from_this();
  }
  void run() {
    auto fd = context_->GetEventFd();
    fd_ = std::make_unique<asio::posix::stream_descriptor>(
        executor_->get_asio_executor(), fd);
    auto listen_event = [](std::shared_ptr<barex_context_t> self,
                           int fd) -> async_simple::coro::Lazy<void> {
      std::error_code ec;
      ELOG_DEBUG << "barex start event loop:" << fd;
      while (true) {
        coro_io::callback_awaitor<std::error_code> awaitor;
        // ELOG_INFO << "barex start event loop waiting:"<<fd;
        ec = co_await awaitor.await_resume([self = self.get(),
                                            fd](auto handler) {
          self->fd_->async_wait(asio::posix::stream_descriptor::wait_read,
                                [handler](const std::error_code& ec) mutable {
                                  handler.set_value_then_resume(ec);
                                });
        });
        if (!ec) {
          int64_t cnt = 0;
          try {
            do {
              cnt += self->context_->ProgressEvents();
            } while (cnt > 0);
            continue;
          } catch (std::exception& e) {
            ELOG_WARN << "barex event loop exit by exception:" << e.what();
          }
        }
        self->context_->Shutdown();
        int cnt;
        try {
          do {
            cnt = self->context_->ProgressEvents();
          } while (cnt > 0);
        } catch (std::exception& e) {
          ELOG_WARN << "barex event loop exit by exception:" << e.what();
        }
        self->context_->WaitStop();
        break;
      }
      co_return;
    };
    listen_event(shared_from_this(), fd)
        .directlyStart(
            [fd](auto&& ec) {
              ELOG_DEBUG << "fd event loop exit:" << fd;
            },
            executor_);
  }
};
namespace detail {

struct helper
    : public std::unordered_map<int, std::shared_ptr<barex_context_t>> {
  using std::unordered_map<int,
                           std::shared_ptr<barex_context_t>>::unordered_map;
  ~helper() {
    for (auto& e : *this) {
      e.second->close();
    }
  }
};
}  // namespace detail
static std::shared_ptr<barex_context_t> get_barex_context(
    coro_io::ExecutorWrapper<>* executor, std::shared_ptr<barex_device_t> dev) {
  auto* barex_ctx_map =
      executor->get_data_with_default<detail::helper>("ylt_barex_ctx");
  if (!barex_ctx_map) {
    return nullptr;
  }
  return barex_context_t::get_static_barex_context_impl(
      *barex_ctx_map, std::move(dev), executor);
}

inline std::error_code barex_acceptor_impl_t::listen() {
  std::vector<accl::barex::XContext*> xctxs;
  xctxs.reserve(ctxs_.size());
  for (auto& ctx : ctxs_) {
    ctx->set_acceptor(this);
    xctxs.push_back(ctx->context_.get());
  }
  accl::barex::XListener* listener = nullptr;
  accl::barex::BarexResult result = accl::barex::XListener::NewInstance(
      listener, 1, port_, accl::barex::TIMER_30S, std::move(xctxs));
  if (result) {
    ELOG_WARN << "init XContext failed:" << result;
    return coro_io::detail::to_std_error_code(result);
  }
  result = listener->Listen();
  if (result) {
    ELOG_WARN << "listen failed:" << result;
    return coro_io::detail::to_std_error_code(result);
  }
  listener_.reset(listener);
  return {};
}

}  // namespace coro_io