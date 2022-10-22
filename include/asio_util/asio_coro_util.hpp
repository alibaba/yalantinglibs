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

class AcceptorAwaiter {
 public:
  AcceptorAwaiter(asio::ip::tcp::acceptor &acceptor,
                  asio::ip::tcp::socket &socket)
      : acceptor_(acceptor), socket_(socket) {}
  bool await_ready() const noexcept { return false; }
  void await_suspend(std::coroutine_handle<> handle) {
    acceptor_.async_accept(socket_, [this, handle](auto ec) mutable {
      ec_ = ec;
      handle.resume();
    });
  }
  auto await_resume() noexcept { return ec_; }
  auto coAwait(async_simple::Executor *executor) noexcept {
    return std::move(*this);
  }

 private:
  asio::ip::tcp::acceptor &acceptor_;
  asio::ip::tcp::socket &socket_;
  std::error_code ec_{};
};

inline async_simple::coro::Lazy<std::error_code> async_accept(
    asio::ip::tcp::acceptor &acceptor, asio::ip::tcp::socket &socket) noexcept {
  co_return co_await AcceptorAwaiter{acceptor, socket};
}

template <typename Socket, typename AsioBuffer>
struct ReadSomeAwaiter {
 public:
  ReadSomeAwaiter(Socket &socket, AsioBuffer &&buffer)
      : socket_(socket), buffer_(std::forward<AsioBuffer>(buffer)) {}
  bool await_ready() { return false; }
  auto await_resume() { return std::make_pair(ec_, size_); }
  void await_suspend(std::coroutine_handle<> handle) {
    socket_.async_read_some(std::move(buffer_),
                            [this, handle](auto ec, auto size) mutable {
                              ec_ = ec;
                              size_ = size;
                              handle.resume();
                            });
  }
  auto coAwait(async_simple::Executor *executor) noexcept {
    return std::move(*this);
  }

 private:
  Socket &socket_;
  AsioBuffer buffer_;

  std::error_code ec_{};
  size_t size_{0};
};

template <typename Socket, typename AsioBuffer>
inline async_simple::coro::Lazy<std::pair<std::error_code, size_t>>
async_read_some(Socket &socket, AsioBuffer &&buffer) noexcept {
  co_return co_await ReadSomeAwaiter{socket, std::move(buffer)};
}

template <typename Socket, typename AsioBuffer>
struct ReadAwaiter {
 public:
  ReadAwaiter(Socket &socket, AsioBuffer &&buffer)
      : socket_(socket), buffer_(std::forward<AsioBuffer>(buffer)) {}
  bool await_ready() { return false; }
  auto await_resume() { return std::make_pair(ec_, size_); }
  void await_suspend(std::coroutine_handle<> handle) {
    asio::async_read(socket_, std::move(buffer_),
                     [this, handle](auto ec, auto size) mutable {
                       ec_ = ec;
                       size_ = size;
                       handle.resume();
                     });
  }
  auto coAwait(async_simple::Executor *executor) noexcept {
    return std::move(*this);
  }

 private:
  Socket &socket_;
  AsioBuffer &&buffer_;

  std::error_code ec_{};
  size_t size_{0};
};

template <typename Socket, typename AsioBuffer>
inline async_simple::coro::Lazy<std::pair<std::error_code, size_t>> async_read(
    Socket &socket, AsioBuffer &&buffer) noexcept {
  co_return co_await ReadAwaiter{socket, std::forward<AsioBuffer>(buffer)};
}

template <typename Socket, typename AsioBuffer>
struct ReadUntilAwaiter {
 public:
  ReadUntilAwaiter(Socket &socket, AsioBuffer &buffer, asio::string_view delim)
      : socket_(socket), buffer_(buffer), delim_(delim) {}
  bool await_ready() { return false; }
  auto await_resume() { return std::make_pair(ec_, size_); }
  void await_suspend(std::coroutine_handle<> handle) {
    asio::async_read_until(socket_, buffer_, delim_,
                           [this, handle](auto ec, auto size) mutable {
                             ec_ = ec;
                             size_ = size;
                             handle.resume();
                           });
  }
  auto coAwait(async_simple::Executor *executor) noexcept {
    return std::move(*this);
  }

 private:
  Socket &socket_;
  AsioBuffer &buffer_;
  asio::string_view delim_;

  std::error_code ec_{};
  size_t size_{0};
};

template <typename Socket, typename AsioBuffer>
inline async_simple::coro::Lazy<std::pair<std::error_code, size_t>>
async_read_until(Socket &socket, AsioBuffer &buffer,
                 asio::string_view delim) noexcept {
  co_return co_await ReadUntilAwaiter{socket, buffer, delim};
}

template <typename Socket, typename AsioBuffer>
struct WriteAwaiter {
 public:
  WriteAwaiter(Socket &socket, AsioBuffer &&buffer)
      : socket_(socket), buffer_(std::move(buffer)) {}
  bool await_ready() { return false; }
  auto await_resume() { return std::make_pair(ec_, size_); }
  void await_suspend(std::coroutine_handle<> handle) {
    asio::async_write(socket_, std::move(buffer_),
                      [this, handle](auto ec, auto size) mutable {
                        ec_ = ec;
                        size_ = size;
                        handle.resume();
                      });
  }
  auto coAwait(async_simple::Executor *executor) noexcept {
    return std::move(*this);
  }

 private:
  Socket &socket_;
  AsioBuffer buffer_;

  std::error_code ec_{};
  size_t size_{0};
};

template <typename Socket, typename AsioBuffer>
inline async_simple::coro::Lazy<std::pair<std::error_code, size_t>> async_write(
    Socket &socket, AsioBuffer &&buffer) noexcept {
  co_return co_await WriteAwaiter{socket, std::move(buffer)};
}

class ConnectAwaiter {
 public:
  ConnectAwaiter(asio::io_context &io_context, asio::ip::tcp::socket &socket,
                 const std::string &host, const std::string &port)
      : io_context_(io_context), socket_(socket), host_(host), port_(port) {}
  bool await_ready() const noexcept { return false; }
  void await_suspend(std::coroutine_handle<> handle) {
    asio::ip::tcp::resolver resolver(io_context_);
    auto endpoints = resolver.resolve(host_, port_);
    asio::async_connect(
        socket_, endpoints,
        [this, handle](std::error_code ec, asio::ip::tcp::endpoint) mutable {
          ec_ = ec;
          handle.resume();
        });
  }
  auto await_resume() noexcept { return ec_; }
  auto coAwait(async_simple::Executor *executor) noexcept {
    return std::move(*this);
  }

 private:
  asio::io_context &io_context_;
  asio::ip::tcp::socket &socket_;
  std::string host_;
  std::string port_;
  std::error_code ec_{};
};

inline async_simple::coro::Lazy<std::error_code> async_connect(
    asio::io_context &io_context, asio::ip::tcp::socket &socket,
    const std::string &host, const std::string &port) noexcept {
  co_return co_await ConnectAwaiter{io_context, socket, host, port};
}

template <typename Socket>
struct CloseAwaiter {
 public:
  CloseAwaiter(Socket &socket) : socket_(socket) {}
  bool await_ready() { return false; }
  void await_resume() {}
  void await_suspend(std::coroutine_handle<> handle) {
    asio::io_context &io_context =
        static_cast<asio::io_context &>(socket_.get_executor().context());
    io_context.post([this, handle]() mutable {
      asio::error_code ignored_ec;
      socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ignored_ec);
      socket_.close(ignored_ec);
      handle.resume();
    });
  }

  auto coAwait(async_simple::Executor *executor) noexcept {
    return std::move(*this);
  }

 private:
  Socket &socket_;
};

template <typename Socket>
inline async_simple::coro::Lazy<void> async_close(Socket &socket) noexcept {
  co_return co_await CloseAwaiter{socket};
}

#ifdef ENABLE_SSL
template <typename Stream>
struct HandshakeAwaiter {
 public:
  HandshakeAwaiter(Stream &ssl_stream,
                   asio::ssl::stream_base::handshake_type type)
      : ssl_stream_(ssl_stream), type_(type) {}
  bool await_ready() { return false; }
  auto await_resume() { return ec_; }
  void await_suspend(std::coroutine_handle<> handle) {
    ssl_stream_->async_handshake(
        type_, [this, handle](const asio::error_code &error) mutable {
          ec_ = error;
          handle.resume();
        });
  }

  auto coAwait(async_simple::Executor *executor) noexcept {
    return std::move(*this);
  }

 private:
  Stream &ssl_stream_;
  asio::ssl::stream_base::handshake_type type_;
  std::error_code ec_{};
};

inline async_simple::coro::Lazy<std::error_code> async_handshake(
    auto &ssl_stream, asio::ssl::stream_base::handshake_type type) noexcept {
  co_return co_await HandshakeAwaiter{ssl_stream, type};
}
#endif

class period_timer : public asio::steady_timer {
  struct TimerAwaiter {
   public:
    TimerAwaiter(period_timer &timer) : timer_(timer) {}
    bool await_ready() { return false; }
    bool await_resume() { return is_timeout_; }
    void await_suspend(std::coroutine_handle<> handle) {
      timer_.async_wait([this, handle](const asio::error_code &ec) mutable {
        if (ec) {
          is_timeout_ = false;
          handle.resume();
          return;
        }

        is_timeout_ = true;
        handle.resume();
      });
    }

    auto coAwait(async_simple::Executor *executor) noexcept {
      return std::move(*this);
    }

   private:
    period_timer &timer_;
    bool is_timeout_ = false;
  };

 public:
  period_timer(asio::io_context &ctx) : asio::steady_timer(ctx) {}
  template <class Rep, class Period>
  period_timer(asio::io_context &ctx,
               const std::chrono::duration<Rep, Period> &timeout_duration)
      : asio::steady_timer(ctx, timeout_duration) {}
  async_simple::coro::Lazy<bool> async_await() noexcept {
    co_return co_await TimerAwaiter{*this};
  }
};

struct sync_task {
  struct promise_type {
    sync_task get_return_object() {
      return {
          std::coroutine_handle<sync_task::promise_type>::from_promise(*this)};
    }
    std::suspend_never initial_suspend() { return {}; }
    std::suspend_never final_suspend() noexcept { return {}; }
    void return_void() {}
    void unhandled_exception() {}
  };

  std::coroutine_handle<sync_task::promise_type> handle_;
};