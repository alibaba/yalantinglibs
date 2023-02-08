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
  void schedule(Func func, Duration dur) override {
    auto timer = std::make_shared<asio::steady_timer>(io_context_, dur);
    timer->async_wait([fn = std::move(func), timer](auto ec) {
      fn();
    });
  }
  asio::io_context &io_context_;
};

template <typename Arg, typename Derived>
class callback_awaitor_base {
 private:
  template <typename Op>
  class callback_awaitor_impl {
   public:
    callback_awaitor_impl(Derived &awaitor, const Op &op) noexcept
        : awaitor(awaitor), op(op) {}
    constexpr bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> handle) noexcept {
      awaitor.coro_ = handle;
      op(awaitor_handler{awaitor});
    }
    auto coAwait(async_simple::Executor *executor) const noexcept {
      return *this;
    }
    decltype(auto) await_resume() noexcept {
      if constexpr (std::is_void_v<Arg>) {
        return;
      }
      else {
        return std::move(awaitor.arg_);
      }
    }

   private:
    Derived &awaitor;
    const Op &op;
  };

 public:
  class awaitor_handler {
   public:
    awaitor_handler(Derived &obj) : obj(obj) {}
    template <typename... Args>
    void set_value_then_resume(Args &&...args) const {
      set_value(std::forward<Args>(args)...);
      resume();
    }
    template <typename... Args>
    void set_value(Args &&...args) const {
      if constexpr (!std::is_void_v<Arg>) {
        obj.arg_ = {std::forward<Args>(args)...};
      }
    }
    void resume() const { obj.coro_.resume(); }

   private:
    Derived &obj;
  };
  template <typename Op>
  callback_awaitor_impl<Op> await_resume(const Op &op) noexcept {
    return callback_awaitor_impl<Op>{static_cast<Derived &>(*this), op};
  }

 private:
  std::coroutine_handle<> coro_;
};

template <typename Arg>
class callback_awaitor
    : public callback_awaitor_base<Arg, callback_awaitor<Arg>> {
  friend class callback_awaitor_base<Arg, callback_awaitor<Arg>>;

 private:
  Arg arg_;
};

template <>
class callback_awaitor<void>
    : public callback_awaitor_base<void, callback_awaitor<void>> {};

inline async_simple::coro::Lazy<std::error_code> async_accept(
    asio::ip::tcp::acceptor &acceptor, asio::ip::tcp::socket &socket) noexcept {
  callback_awaitor<std::error_code> awaitor;

  co_return co_await awaitor.await_resume([&](auto handler) {
    acceptor.async_accept(socket, [&, handler](const auto &ec) mutable {
      handler.set_value_then_resume(ec);
    });
  });
}

template <typename Socket, typename AsioBuffer>
inline async_simple::coro::Lazy<std::pair<std::error_code, size_t>>
async_read_some(Socket &socket, AsioBuffer &&buffer) noexcept {
  callback_awaitor<std::pair<std::error_code, size_t>> awaitor;
  co_return co_await awaitor.await_resume([&](auto handler) {
    socket.async_read_some(buffer, [&, handler](const auto &ec, auto size) {
      handler.set_value_then_resume(ec, size);
    });
  });
}
template <typename Socket, typename AsioBuffer>
inline async_simple::coro::Lazy<std::pair<std::error_code, size_t>> async_read(
    Socket &socket, AsioBuffer &&buffer) noexcept {
  callback_awaitor<std::pair<std::error_code, size_t>> awaitor;
  co_return co_await awaitor.await_resume([&](auto handler) {
    asio::async_read(socket, buffer, [&, handler](const auto &ec, auto size) {
      handler.set_value_then_resume(ec, size);
    });
  });
}

template <typename Socket, typename AsioBuffer>
inline async_simple::coro::Lazy<std::pair<std::error_code, size_t>>
async_read_until(Socket &socket, AsioBuffer &buffer,
                 asio::string_view delim) noexcept {
  callback_awaitor<std::pair<std::error_code, size_t>> awaitor;
  co_return co_await awaitor.await_resume([&](auto handler) {
    asio::async_read_until(socket, buffer, delim,
                           [&, handler](const auto &ec, auto size) {
                             handler.set_value_then_resume(ec, size);
                           });
  });
}

template <typename Socket, typename AsioBuffer>
inline async_simple::coro::Lazy<std::pair<std::error_code, size_t>> async_write(
    Socket &socket, AsioBuffer &&buffer) noexcept {
  callback_awaitor<std::pair<std::error_code, size_t>> awaitor;
  co_return co_await awaitor.await_resume([&](auto handler) {
    asio::async_write(socket, buffer, [&, handler](const auto &ec, auto size) {
      handler.set_value_then_resume(ec, size);
    });
  });
}

inline async_simple::coro::Lazy<std::error_code> async_connect(
    asio::io_context &io_context, asio::ip::tcp::socket &socket,
    const std::string &host, const std::string &port) noexcept {
  callback_awaitor<std::error_code> awaitor;
  asio::ip::tcp::resolver resolver(io_context);
  asio::ip::tcp::resolver::iterator iterator;
  auto ec = co_await awaitor.await_resume([&](auto handler) {
    resolver.async_resolve(host, port, [&, handler](auto ec, auto it) {
      iterator = it;
      handler.set_value_then_resume(ec);
    });
  });

  if (ec) {
    co_return ec;
  }

  co_return co_await awaitor.await_resume([&](auto handler) {
    asio::async_connect(socket, iterator,
                        [&, handler](const auto &ec, const auto &) mutable {
                          handler.set_value_then_resume(ec);
                        });
  });
}

template <typename Socket>
inline async_simple::coro::Lazy<void> async_close(Socket &socket) noexcept {
  callback_awaitor<void> awaitor;
  asio::io_context &io_context =
      static_cast<asio::io_context &>(socket.get_executor().context());
  co_return co_await awaitor.await_resume([&](auto handler) {
    asio::post(io_context, [&, handler]() {
      asio::error_code ignored_ec;
      socket.shutdown(asio::ip::tcp::socket::shutdown_both, ignored_ec);
      socket.close(ignored_ec);
      handler.resume();
    });
  });
}

#ifdef ENABLE_SSL
inline async_simple::coro::Lazy<std::error_code> async_handshake(
    auto &ssl_stream, asio::ssl::stream_base::handshake_type type) noexcept {
  callback_awaitor<std::error_code> awaitor;
  co_return co_await awaitor.await_resume([&](auto handler) {
    ssl_stream->async_handshake(type, [&, handler](const auto &ec) {
      handler.set_value_then_resume(ec);
    });
  });
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
    callback_awaitor<bool> awaitor;
    co_return co_await awaitor.await_resume([&](auto handler) {
      this->async_wait([&, handler](const auto &ec) {
        handler.set_value_then_resume(!ec);
      });
    });
  }
};

}  // namespace asio_util
