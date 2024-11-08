/*
 * Copyright (c) 2023, Alibaba Group Holding Limited;
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

#include <async_simple/Executor.h>
#include <async_simple/coro/Collect.h>
#include <async_simple/coro/Lazy.h>
#include <async_simple/coro/Sleep.h>
#include <async_simple/coro/SyncAwait.h>

#if defined(YLT_ENABLE_SSL) || defined(CINATRA_ENABLE_SSL)
#include <asio/ssl.hpp>
#endif

#include <asio/connect.hpp>
#include <asio/experimental/channel.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/read.hpp>
#include <asio/read_at.hpp>
#include <asio/read_until.hpp>
#include <asio/write.hpp>
#include <asio/write_at.hpp>
#include <chrono>
#include <deque>

#include "io_context_pool.hpp"
#if __has_include("ylt/util/type_traits.h")
#include "ylt/util/type_traits.h"
#else
#include "../util/type_traits.h"
#endif
#ifdef __linux__
#include <sys/sendfile.h>
#endif

namespace coro_io {
template <typename T>
constexpr inline bool is_lazy_v =
    util::is_specialization_v<std::remove_cvref_t<T>, async_simple::coro::Lazy>;

template <typename Arg, typename Derived>
class callback_awaitor_base {
 private:
  template <typename Op>
  class callback_awaitor_impl {
   public:
    callback_awaitor_impl(Derived &awaitor, Op &op) noexcept
        : awaitor(awaitor), op(op) {}
    constexpr bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> handle) noexcept {
      awaitor.coro_ = handle;
      op(awaitor_handler{&awaitor});
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
    Op &op;
  };

 public:
  class awaitor_handler {
   public:
    awaitor_handler(Derived *obj) : obj(obj) {}
    awaitor_handler(awaitor_handler &&) = default;
    awaitor_handler(const awaitor_handler &) = default;
    awaitor_handler &operator=(const awaitor_handler &) = default;
    awaitor_handler &operator=(awaitor_handler &&) = default;
    template <typename... Args>
    void set_value_then_resume(Args &&...args) const {
      set_value(std::forward<Args>(args)...);
      resume();
    }
    template <typename... Args>
    void set_value(Args &&...args) const {
      if constexpr (!std::is_void_v<Arg>) {
        obj->arg_ = {std::forward<Args>(args)...};
      }
    }
    void resume() const { obj->coro_.resume(); }

   private:
    Derived *obj;
  };
  template <typename Op>
  callback_awaitor_impl<Op> await_resume(Op &&op) noexcept {
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
inline async_simple::coro::Lazy<std::pair<std::error_code, size_t>>
async_read_at(uint64_t offset, Socket &socket, AsioBuffer &&buffer) noexcept {
  callback_awaitor<std::pair<std::error_code, size_t>> awaitor;
  co_return co_await awaitor.await_resume([&](auto handler) {
    asio::async_read_at(socket, offset, buffer,
                        [&, handler](const auto &ec, auto size) {
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
inline async_simple::coro::Lazy<std::pair<std::error_code, size_t>> async_read(
    Socket &socket, AsioBuffer &buffer, size_t size_to_read) noexcept {
  callback_awaitor<std::pair<std::error_code, size_t>> awaitor;
  co_return co_await awaitor.await_resume([&](auto handler) {
    asio::async_read(socket, buffer, asio::transfer_exactly(size_to_read),
                     [&, handler](const auto &ec, auto size) {
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

template <typename Socket, typename AsioBuffer>
inline async_simple::coro::Lazy<std::pair<std::error_code, size_t>>
async_write_some(Socket &socket, AsioBuffer &&buffer) noexcept {
  callback_awaitor<std::pair<std::error_code, size_t>> awaitor;
  co_return co_await awaitor.await_resume([&](auto handler) {
    socket.async_write_some(buffer, [&, handler](const auto &ec, auto size) {
      handler.set_value_then_resume(ec, size);
    });
  });
}

template <typename Socket, typename AsioBuffer>
inline async_simple::coro::Lazy<std::pair<std::error_code, size_t>>
async_write_at(uint64_t offset, Socket &socket, AsioBuffer &&buffer) noexcept {
  callback_awaitor<std::pair<std::error_code, size_t>> awaitor;
  co_return co_await awaitor.await_resume([&](auto handler) {
    asio::async_write_at(socket, offset, buffer,
                         [&, handler](const auto &ec, auto size) {
                           handler.set_value_then_resume(ec, size);
                         });
  });
}

template <typename executor_t>
inline async_simple::coro::Lazy<std::error_code> async_connect(
    executor_t *executor, asio::ip::tcp::socket &socket,
    const std::string &host, const std::string &port) noexcept {
  callback_awaitor<std::error_code> awaitor;
  asio::ip::tcp::resolver resolver(executor->get_asio_executor());
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
  auto executor = socket.get_executor();
  co_return co_await awaitor.await_resume([&](auto handler) {
    asio::post(executor, [&, handler]() {
      asio::error_code ignored_ec;
      socket.shutdown(asio::ip::tcp::socket::shutdown_both, ignored_ec);
      socket.close(ignored_ec);
      handler.resume();
    });
  });
}

#if defined(YLT_ENABLE_SSL) || defined(CINATRA_ENABLE_SSL)
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
  using asio::steady_timer::steady_timer;
  template <typename T>
  period_timer(coro_io::ExecutorWrapper<T> *executor)
      : asio::steady_timer(executor->get_asio_executor()) {}

  async_simple::coro::Lazy<bool> async_await() noexcept {
    callback_awaitor<bool> awaitor;

    co_return co_await awaitor.await_resume([&](auto handler) {
      this->async_wait([&, handler](const auto &ec) {
        handler.set_value_then_resume(!ec);
      });
    });
  }
};

template <typename Duration, typename Executor>
inline async_simple::coro::Lazy<void> sleep_for(const Duration &d,
                                                Executor *e) {
  coro_io::period_timer timer(e);
  timer.expires_after(d);
  co_await timer.async_await();
}
template <typename Duration>
inline async_simple::coro::Lazy<void> sleep_for(Duration d) {
  if (auto executor = co_await async_simple::CurrentExecutor();
      executor != nullptr) {
    co_await async_simple::coro::sleep(d);
  }
  else {
    co_return co_await sleep_for(d,
                                 coro_io::g_io_context_pool().get_executor());
  }
}

template <typename R, typename Func, typename Executor>
struct post_helper {
  void operator()(auto handler) {
    asio::post(e, [this, handler]() {
      try {
        if constexpr (std::is_same_v<R, async_simple::Try<void>>) {
          func();
          handler.resume();
        }
        else {
          auto r = func();
          handler.set_value_then_resume(std::move(r));
        }
      } catch (const std::exception &e) {
        R er;
        er.setException(std::current_exception());
        handler.set_value_then_resume(std::move(er));
      }
    });
  }
  Executor e;
  Func func;
};

template <typename Func, typename Executor>
inline async_simple::coro::Lazy<
    async_simple::Try<typename util::function_traits<Func>::return_type>>
post(Func func, Executor executor) {
  using R =
      async_simple::Try<typename util::function_traits<Func>::return_type>;

  callback_awaitor<R> awaitor;
  post_helper<R, Func, Executor> helper{executor, std::move(func)};
  co_return co_await awaitor.await_resume(helper);
}

template <typename Func>
inline async_simple::coro::Lazy<
    async_simple::Try<typename util::function_traits<Func>::return_type>>
post(Func func,
     coro_io::ExecutorWrapper<> *e = coro_io::get_global_block_executor()) {
  co_return co_await post(std::move(func), e->get_asio_executor());
}

template <typename R>
struct channel : public asio::experimental::channel<void(std::error_code, R)> {
  using return_type = R;
  using ValueType = std::pair<std::error_code, R>;
  using asio::experimental::channel<void(std::error_code, R)>::channel;
  channel(coro_io::ExecutorWrapper<> *executor, size_t capacity)
      : executor_(executor),
        asio::experimental::channel<void(std::error_code, R)>(
            executor->get_asio_executor(), capacity) {}
  auto get_executor() { return executor_; }

 private:
  coro_io::ExecutorWrapper<> *executor_;
};

template <typename R>
inline channel<R> create_channel(
    size_t capacity,
    coro_io::ExecutorWrapper<> *executor = coro_io::get_global_executor()) {
  return channel<R>(executor, capacity);
}

template <typename R>
inline auto create_shared_channel(
    size_t capacity,
    coro_io::ExecutorWrapper<> *executor = coro_io::get_global_executor()) {
  return std::make_shared<channel<R>>(executor, capacity);
}

template <typename T>
inline async_simple::coro::Lazy<std::error_code> async_send(
    asio::experimental::channel<void(std::error_code, T)> &channel, T val) {
  bool r = channel.try_send(std::error_code{}, val);
  if (r) {
    co_return std::error_code{};
  }
  callback_awaitor<std::error_code> awaitor;
  co_return co_await awaitor.await_resume(
      [&, val = std::move(val)](auto handler) {
        channel.async_send({}, std::move(val), [handler](const auto &ec) {
          handler.set_value_then_resume(ec);
        });
      });
}

template <typename Channel>
async_simple::coro::Lazy<std::pair<
    std::error_code,
    typename Channel::return_type>> inline async_receive(Channel &channel) {
  using value_type = typename Channel::return_type;
  value_type val;
  bool r = channel.try_receive([&val](std::error_code, value_type result) {
    val = result;
  });
  if (r) {
    co_return std::make_pair(std::error_code{}, val);
  }
  callback_awaitor<std::pair<std::error_code, typename Channel::return_type>>
      awaitor;
  co_return co_await awaitor.await_resume([&](auto handler) {
    channel.async_receive([handler](auto ec, auto val) {
      handler.set_value_then_resume(std::make_pair(ec, std::move(val)));
    });
  });
}

template <typename T>
inline decltype(auto) select_impl(T &pair) {
  using Func = std::tuple_element_t<1, std::remove_cvref_t<T>>;
  using ValueType =
      typename std::tuple_element_t<0, std::remove_cvref_t<T>>::ValueType;
  using return_type = std::invoke_result_t<Func, async_simple::Try<ValueType>>;

  auto &callback = std::get<1>(pair);
  if constexpr (coro_io::is_lazy_v<return_type>) {
    auto executor = std::get<0>(pair).getExecutor();
    return std::make_pair(
        std::move(std::get<0>(pair)),
        [executor, callback = std::move(callback)](auto &&val) {
          if (executor) {
            callback(std::move(val)).via(executor).start([](auto &&) {
            });
          }
          else {
            callback(std::move(val)).start([](auto &&) {
            });
          }
        });
  }
  else {
    return pair;
  }
}

template <typename... T>
inline auto select(T &&...args) {
  return async_simple::coro::collectAny(select_impl(args)...);
}

template <typename T, typename Callback>
inline auto select(std::vector<T> vec, Callback callback) {
  if constexpr (coro_io::is_lazy_v<Callback>) {
    std::vector<async_simple::Executor *> executors;
    for (auto &lazy : vec) {
      executors.push_back(lazy.getExecutor());
    }

    return async_simple::coro::collectAny(
        std::move(vec),
        [executors, callback = std::move(callback)](size_t index, auto &&val) {
          auto executor = executors[index];
          if (executor) {
            callback(index, std::move(val)).via(executor).start([](auto &&) {
            });
          }
          else {
            callback(index, std::move(val)).start([](auto &&) {
            });
          }
        });
  }
  else {
    return async_simple::coro::collectAny(std::move(vec), std::move(callback));
  }
}

template <typename Socket, typename AsioBuffer>
std::pair<asio::error_code, size_t> read_some(Socket &sock,
                                              AsioBuffer &&buffer) {
  asio::error_code error;
  size_t length = sock.read_some(std::forward<AsioBuffer>(buffer), error);
  return std::make_pair(error, length);
}

template <typename Socket, typename AsioBuffer>
std::pair<asio::error_code, size_t> read(Socket &sock, AsioBuffer &&buffer) {
  asio::error_code error;
  size_t length = asio::read(sock, buffer, error);
  return std::make_pair(error, length);
}

template <typename Socket, typename AsioBuffer>
std::pair<asio::error_code, size_t> write(Socket &sock, AsioBuffer &&buffer) {
  asio::error_code error;
  auto length = asio::write(sock, std::forward<AsioBuffer>(buffer), error);
  return std::make_pair(error, length);
}

inline std::error_code accept(asio::ip::tcp::acceptor &a,
                              asio::ip::tcp::socket &socket) {
  std::error_code error;
  a.accept(socket, error);
  return error;
}

template <typename executor_t>
inline std::error_code connect(executor_t &executor,
                               asio::ip::tcp::socket &socket,
                               const std::string &host,
                               const std::string &port) {
  asio::ip::tcp::resolver resolver(executor);
  std::error_code error;
  auto endpoints = resolver.resolve(host, port, error);
  if (error) {
    return error;
  }
  asio::connect(socket, endpoints, error);
  return error;
}

#ifdef __linux__
// this help us ignore SIGPIPE when send data to a unexpected closed socket.
inline auto pipe_signal_handler = [] {
  std::signal(SIGPIPE, SIG_IGN);
  return 0;
}();

// FIXME: this function may not thread-safe if it not running in socket's
// executor
inline async_simple::coro::Lazy<std::pair<std::error_code, std::size_t>>
async_sendfile(asio::ip::tcp::socket &socket, int fd, off_t offset,
               size_t size) noexcept {
  std::error_code ec;
  std::size_t least_bytes = size;
  if (!ec) [[likely]] {
    if (!socket.native_non_blocking()) {
      socket.native_non_blocking(true, ec);
      if (ec) {
        co_return std::pair{ec, 0};
      }
    }
    while (true) {
      // Try the system call.
      errno = 0;
      ssize_t n = ::sendfile(socket.native_handle(), fd, &offset,
                             std::min(std::size_t{65536}, least_bytes));
      ec = asio::error_code(n < 0 ? errno : 0,
                            asio::error::get_system_category());
      least_bytes -= ec ? 0 : n;
      // total_bytes_transferred += ec ? 0 : n;
      // Retry operation immediately if interrupted by signal.
      if (ec == asio::error::interrupted) [[unlikely]]
        continue;
      // Check if we need to run the operation again.
      if (ec == asio::error::would_block || ec == asio::error::try_again)
          [[unlikely]] {
        callback_awaitor<std::error_code> non_block_awaitor;
        // We have to wait for the socket to become ready again.
        ec = co_await non_block_awaitor.await_resume([&](auto handler) {
          socket.async_wait(asio::ip::tcp::socket::wait_write,
                            [handler](const auto &ec) {
                              handler.set_value_then_resume(ec);
                            });
        });
      }
      if (ec || n == 0 || least_bytes == 0) [[unlikely]] {  // End of File
        break;
      }
      // Loop around to try calling sendfile again.
    }
  }
  co_return std::pair{ec, size - least_bytes};
}
#endif
}  // namespace coro_io
