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
#include <async_simple/coro/Lazy.h>
#include <async_simple/coro/SyncAwait.h>
#include <async_simple/executors/SimpleExecutor.h>

#include <asio.hpp>
#include <chrono>

#include "async_simple/Future.h"
#include "async_simple/coro/FutureAwaiter.h"

#ifdef ENABLE_SSL
#include <asio/ssl.hpp>
#endif

namespace asio_util {

class AsioExecutor : public async_simple::Executor {
 public:
  AsioExecutor(asio::io_context &io_context) : io_context_(io_context) {}

  virtual bool schedule(Func func) override {
    asio::post(io_context_, std::move(func));
    return true;
  }

  bool currentThreadInExecutor() const override { return false; }

  async_simple::ExecutorStat stat() const override { return {}; }

 private:
  asio::io_context &io_context_;
};

template <typename Arg>
class callback_promise;

template <typename Arg>
class await_future {
 public:
  await_future(callback_promise<Arg> &promise) : promise(promise) {}
  bool await_ready() const noexcept;
  void await_suspend(std::coroutine_handle<> handle) noexcept;
  auto coAwait(async_simple::Executor *executor) const noexcept {
    return *this;
  }
  decltype(auto) await_resume() noexcept;

 private:
  callback_promise<Arg> &promise;
};

template <typename Arg, typename Derived>
class callback_promise_base {
 public:
  callback_promise_base() : future(static_cast<Derived &>(*this)) {}
  template <typename... Args>
  void set_value_then_resume(Args &&...args) {
    set_value(std::forward<Args>(args)...);
    resume();
  }
  template <typename... Args>
  void set_value(Args &&...args) {
    static_cast<Derived &>(*this).set_value(std::forward<Args>(args)...);
  }
  // warning: this function is not thread-safe when running in different threads
  // with co_await promise.get_future();
  void resume() {
    if (coro_ == nullptr) [[unlikely]] {
      is_ready_ = true;
    }
    else {
      coro_.resume();
    }
  }
  const await_future<Arg> &get_future() const noexcept { return future; }

  friend class await_future<Arg>;

 private:
  bool is_ready_ = false;
  std::coroutine_handle<> coro_;
  await_future<Arg> future;
};

template <typename Arg>
class callback_promise
    : public callback_promise_base<Arg, callback_promise<Arg>> {
 public:
  template <typename... Args>
  void set_value(Args &&...args) {
    arg_ = {std::forward<Args>(args)...};
  }
  friend class await_future<Arg>;

 private:
  Arg arg_;
};

template <>
class callback_promise<void>
    : public callback_promise_base<void, callback_promise<void>> {
 public:
  void set_value(){};
};

template <typename Arg>
void inline await_future<Arg>::await_suspend(
    std::coroutine_handle<> handle) noexcept {
  promise.coro_ = handle;
}

template <typename Arg>
decltype(auto) inline await_future<Arg>::await_resume() noexcept {
  return std::move(promise.arg_);
}
template <>
decltype(auto) inline await_future<void>::await_resume() noexcept {
  return;
}
template <typename Arg>
bool await_future<Arg>::await_ready() const noexcept {
  return promise.is_ready_;
}

inline async_simple::coro::Lazy<std::error_code> async_accept(
    asio::ip::tcp::acceptor &acceptor, asio::ip::tcp::socket &socket) noexcept {
  callback_promise<std::error_code> promise;
  acceptor.async_accept(socket, [&](const auto &ec) {
    promise.set_value_then_resume(ec);
  });
  co_return co_await promise.get_future();
}

template <typename Socket, typename AsioBuffer>
inline async_simple::coro::Lazy<std::pair<std::error_code, size_t>>
async_read_some(Socket &socket, AsioBuffer &&buffer) noexcept {
  callback_promise<std::pair<std::error_code, size_t>> promise;
  socket.async_read_some(buffer, [&](const auto &ec, auto size) {
    promise.set_value_then_resume(ec, size);
  });
  co_return co_await promise.get_future();
}
template <typename Socket, typename AsioBuffer>
inline async_simple::coro::Lazy<std::pair<std::error_code, size_t>> async_read(
    Socket &socket, AsioBuffer &&buffer) noexcept {
  callback_promise<std::pair<std::error_code, size_t>> promise;
  asio::async_read(socket, buffer, [&](const auto &ec, auto size) {
    promise.set_value_then_resume(ec, size);
  });
  co_return co_await promise.get_future();
}

template <typename Socket, typename AsioBuffer>
inline async_simple::coro::Lazy<std::pair<std::error_code, size_t>>
async_read_until(Socket &socket, AsioBuffer &buffer,
                 asio::string_view delim) noexcept {
  callback_promise<std::pair<std::error_code, size_t>> promise;
  asio::async_read_until(socket, buffer, delim, [&](const auto &ec, auto size) {
    promise.set_value_then_resume(ec, size);
  });
  co_return co_await promise.get_future();
}

template <typename Socket, typename AsioBuffer>
inline async_simple::coro::Lazy<std::pair<std::error_code, size_t>> async_write(
    Socket &socket, AsioBuffer &&buffer) noexcept {
  callback_promise<std::pair<std::error_code, size_t>> promise;
  asio::async_write(socket, buffer, [&](const auto &ec, auto size) {
    promise.set_value_then_resume(ec, size);
  });
  co_return co_await promise.get_future();
}

inline async_simple::coro::Lazy<std::error_code> async_connect(
    asio::io_context &io_context, asio::ip::tcp::socket &socket,
    const std::string &host, const std::string &port) noexcept {
  callback_promise<std::error_code> promise;
  asio::ip::tcp::resolver resolver(io_context);
  auto endpoints = resolver.resolve(host, port);
  asio::async_connect(socket, endpoints,
                      [&](const auto &ec, const auto &) mutable {
                        promise.set_value_then_resume(ec);
                      });
  co_return co_await promise.get_future();
}

template <typename Socket>
inline async_simple::coro::Lazy<void> async_close(Socket &socket) noexcept {
  async_simple::Promise<std::monostate> promise;
  asio::io_context &io_context =
      static_cast<asio::io_context &>(socket.get_executor().context());
  asio::post(io_context, [&]() {
    asio::error_code ignored_ec;
    socket.shutdown(asio::ip::tcp::socket::shutdown_both, ignored_ec);
    socket.close(ignored_ec);
    promise.setValue(std::monostate{});
  });
  co_await promise.getFuture();
  co_return;
}

#ifdef ENABLE_SSL
inline async_simple::coro::Lazy<std::error_code> async_handshake(
    auto &ssl_stream, asio::ssl::stream_base::handshake_type type) noexcept {
  callback_promise<std::error_code> promise;
  ssl_stream->async_handshake(type, [&](const auto &ec) {
    promise.set_value_then_resume(ec);
  });
  co_return co_await promise.get_future();
}
#endif

class period_timer : public asio::steady_timer {
 public:
  period_timer(asio::io_context &ctx) : asio::steady_timer(ctx) {}
  template <class Rep, class Period>
  period_timer(asio::io_context &ctx,
               const std::chrono::duration<Rep, Period> &timeout_duration)
      : asio::steady_timer(ctx, timeout_duration) {}
  async_simple::coro::Lazy<bool> async_await() noexcept {
    callback_promise<bool> promise;
    this->async_wait([&](const asio::error_code &ec) {
      promise.set_value_then_resume(!ec);
    });
    co_return co_await promise.get_future();
  }
};

}  // namespace asio_util
