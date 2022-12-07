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
#include <any>
#include <atomic>
#include <cstdint>
#include <future>
#include <memory>
#include <system_error>
#include <utility>
#include <variant>
#include <vector>

#include "asio_util/asio_coro_util.hpp"
#include "asio_util/asio_util.hpp"
#include "async_simple/coro/SyncAwait.h"
#include "logging/easylog.hpp"
#include "router.hpp"
#include "router_impl.hpp"
#include "rpc_protocol.h"
#include "struct_pack/struct_pack.hpp"
#ifdef UNIT_TEST_INJECT
#include "inject_action.hpp"
#endif
namespace coro_rpc {
/*!
 * TODO: add doc
 */
class coro_connection : public std::enable_shared_from_this<coro_connection> {
 public:
  /*!
   *
   * @param io_context
   * @param socket
   * @param timeout_duration
   */
  coro_connection(asio::io_context &io_context, asio::ip::tcp::socket socket,
                  std::chrono::steady_clock::duration timeout_duration =
                      std::chrono::seconds(0))
      : io_context_(io_context),
        executor_(io_context),
        socket_(std::move(socket)),
        rsp_err_(std::errc{}),
        timer_(io_context) {
    body_.resize(body_size_);
    if (timeout_duration == std::chrono::seconds(0)) {
      return;
    }

    enable_check_timeout_ = true;
    keep_alive_timeout_duration_ = timeout_duration;
  }

  ~coro_connection() {
    if (!has_closed_) {
#ifdef UNIT_TEST_INJECT
      easylog::info("~async_connection conn_id {}, client_id {}", conn_id_,
                    client_id_);
#endif
      async_simple::coro::syncAwait(close());
    }
  }

#ifdef ENABLE_SSL
  void init_ssl(asio::ssl::context &ssl_ctx) {
    ssl_stream_ = std::make_unique<asio::ssl::stream<asio::ip::tcp::socket &>>(
        socket_, ssl_ctx);
    use_ssl_ = true;
  }
#endif

  async_simple::coro::Lazy<void> start() noexcept {
#ifdef ENABLE_SSL
    if (use_ssl_) {
      assert(ssl_stream_);
      easylog::info("begin to handshake conn_id {}", conn_id_);
      reset_timer();
      auto shake_ec = co_await asio_util::async_handshake(
          ssl_stream_, asio::ssl::stream_base::server);
      cancel_timer();
      if (shake_ec) {
        easylog::error("handshake failed:{} conn_id {}", shake_ec.message(),
                       conn_id_);
        co_await close();
        co_return;
      }
      easylog::info("handshake ok conn_id {}", conn_id_);
      co_await start_impl(*ssl_stream_);
    }
    else {
#endif
      co_await start_impl(socket_);
#ifdef ENABLE_SSL
    }
#endif
    if (quit_callback_) {
      quit_callback_(conn_id_);
    }
    promise_.set_value();
  }
  template <typename Socket>
  async_simple::coro::Lazy<void> start_impl(Socket &socket) noexcept {
    char head_[RPC_HEAD_LEN];
    rpc_header header{};
    auto self = shared_from_this();
    while (true) {
      reset_timer();
      auto ret = co_await asio_util::async_read(
          socket, asio::buffer(head_, RPC_HEAD_LEN));
      cancel_timer();
      // `co_await async_read` uses asio::async_read underlying.
      // If eof occurred, the bytes_transferred of `co_await async_read` must
      // less than RPC_HEAD_LEN. Incomplete data will be discarded.
      // So, no special handling of eof is required.
      if (ret.first) {
        log((std::errc)ret.first.value());
        co_await close();
        co_return;
      }
      assert(ret.second == RPC_HEAD_LEN);
      auto errc = struct_pack::deserialize_to(header, head_, RPC_HEAD_LEN);
      if (errc != struct_pack::errc::ok) [[unlikely]] {
        log(std::errc::protocol_error);
        co_await close();
        co_return;
      }

#ifdef UNIT_TEST_INJECT
      client_id_ = header.seq_num;
      easylog::info("conn_id {}, client_id {}", conn_id_, client_id_);
#endif

#ifdef UNIT_TEST_INJECT
      if (g_action == inject_action::close_socket_after_read_header) {
        easylog::warn(
            "inject action: close_socket_after_read_header, conn_id {}, "
            "client_id {}",
            conn_id_, client_id_);
        co_await close();
        co_return;
      }
#endif
      if (header.magic != magic_number) [[unlikely]] {
        easylog::error("bad magic number, conn_id {}", conn_id_);
        co_await close();
        co_return;
      }
      if (header.length == 0) [[unlikely]] {
        easylog::info("bad length:{}, conn_id {}", header.length, conn_id_);
        co_await close();
        co_return;
      }

      if (header.length > body_size_) {
        body_size_ = header.length;
        body_.resize(body_size_);
      }

      ret = co_await asio_util::async_read(
          socket, asio::buffer(body_.data(), header.length));
      if (ret.first) [[unlikely]] {
        easylog::info("read error:{}, conn_id {}", ret.first.message(),
                      conn_id_);
        co_await close();
        co_return;
      }

      std::pair<std::errc, std::vector<char>> pair{};
      auto handler = internal::get_handler({body_.data(), ret.second});
      if (!handler) {
        auto coro_handler =
            internal::get_coro_handler({body_.data(), ret.second});
        pair = co_await internal::route_coro(coro_handler,
                                             {body_.data(), ret.second}, self);
      }
      else {
        pair = internal::route(handler, {body_.data(), ret.second}, self);
      }

      auto &[err, buf] = pair;
      if (delay_) {
        delay_ = false;
        continue;
      }
      *((uint32_t *)buf.data()) = buf.size() - RESPONSE_HEADER_LEN;
#ifdef UNIT_TEST_INJECT
      if (g_action == inject_action::close_socket_after_send_length) {
        easylog::warn(
            "inject action: close_socket_after_send_length conn_id {}, "
            "client_id {}",
            conn_id_, client_id_);
        co_await asio_util::async_write(
            socket, asio::buffer(buf.data(), RESPONSE_HEADER_LEN));
        co_await close();
        co_return;
      }
      if (g_action == inject_action::server_send_bad_rpc_result) {
        easylog::warn(
            "inject action: server_send_bad_rpc_result conn_id {}, client_id "
            "{}",
            conn_id_, client_id_);
        buf[RESPONSE_HEADER_LEN + 1] = (buf[RESPONSE_HEADER_LEN + 1] + 1);
      }
#endif
      if (rsp_err_ == err_ok) [[likely]] {
        if (err != err_ok) [[unlikely]] {
          rsp_err_ = err;
        }
        write_queue_.push_back(std::move(buf));
        if (write_queue_.size() == 1) {
          send_data().start([ec = shared_from_this()](auto &&) {
          });
        }
        if (err != err_ok) [[unlikely]] {
          co_return;
        }
      }
    }
  }
  /*!
   * send `ret` to RPC client
   *
   * @tparam R message type
   * @param ret object of message type
   */
  template <typename R>
  void response_msg(const R &ret) {
    if (has_closed()) [[unlikely]] {
      easylog::debug("response_msg failed: connection has been closed");
      return;
    }

    auto buf = struct_pack::serialize_with_offset(RESPONSE_HEADER_LEN, ret);
    *((uint32_t *)buf.data()) = buf.size() - RESPONSE_HEADER_LEN;

    auto self = shared_from_this();
    response(std::move(buf), std::move(self)).via(&executor_).detach();
  }

  /*!
   * send empty message to RPC client
   */
  void response_msg() { response_msg(std::monostate{}); }

  void set_delay(bool b) { delay_ = b; }

  /*!
   * Check the connection has closed or not
   *
   * @return true if closed, otherwise false.
   */
  bool has_closed() const { return has_closed_; }

  void wait_quit() { promise_.get_future().wait(); }
  /*!
   * close connection synchronously
   *
   * If the connection has already closed, nothing will happen.
   *
   * @param close_ssl whether to close the ssl stream
   */
  void sync_close(bool close_ssl = true) {
    if (has_closed_) {
      return;
    }

#ifdef UNIT_TEST_INJECT
    easylog::info("sync_close conn_id {}, client_id {}", conn_id_, client_id_);
#else
    easylog::info("sync_close conn_id {}", conn_id_);
#endif
    std::promise<void> promise;
    close().via(&executor_).start([&](auto &&) {
      promise.set_value();
    });
    promise.get_future().wait();

    io_context_.poll();
  }

  using QuitCallback = std::function<void(const uint64_t &conn_id)>;
  void set_quit_callback(QuitCallback callback, uint64_t conn_id) {
    quit_callback_ = std::move(callback);
    conn_id_ = conn_id;
  }

  template <typename T>
  void set_tag(T &&tag) {
    tag_ = std::forward<T>(tag);
  }

  std::any get_tag() { return tag_; }

 private:
  async_simple::coro::Lazy<void> response(std::vector<char> buf,
                                          auto self) noexcept {
    if (has_closed()) [[unlikely]] {
      easylog::debug("response_msg failed: connection has been closed");
      co_return;
    }
#ifdef UNIT_TEST_INJECT
    if (g_action == inject_action::close_socket_after_send_length) {
      easylog::warn("inject action: close_socket_after_send_length");
      buf.resize(RESPONSE_HEADER_LEN);
    }
#endif
    write_queue_.push_back(std::move(buf));
    if (write_queue_.size() > 1) {
      co_return;
    }
    co_await send_data();
  }

  void log(std::errc err, source_location loc = {}) {
#ifdef UNIT_TEST_INJECT
    easylog::info(loc, "close conn_id {}, client_id {}, reason {}", conn_id_,
                  client_id_, std::make_error_code(err).message());
#else
    easylog::info(loc, "close conn_id {}, reason {}", conn_id_,
                  std::make_error_code(err).message());
#endif
  }

  async_simple::coro::Lazy<void> send_data(bool close_ssl = true) {
    std::pair<std::error_code, size_t> ret;
    while (!write_queue_.empty()) {
      auto &msg = write_queue_.front();
#ifdef UNIT_TEST_INJECT
      if (g_action == inject_action::force_inject_connection_close_socket) {
        easylog::warn(
            "inject action: force_inject_connection_close_socket, conn_id {}, "
            "client_id {}",
            conn_id_, client_id_);
        co_await close(false);
        co_return;
      }
#endif
#ifdef ENABLE_SSL
      if (use_ssl_) {
        assert(ssl_stream_);
        ret = co_await asio_util::async_write(*ssl_stream_, asio::buffer(msg));
      }
      else {
#endif
        ret = co_await asio_util::async_write(socket_, asio::buffer(msg));
#ifdef ENABLE_SSL
      }
#endif
      if (ret.first) [[unlikely]] {
        log((std::errc)ret.first.value());
        co_await close(close_ssl);
        co_return;
      }
      write_queue_.pop_front();
    }
    if (rsp_err_ != err_ok) [[unlikely]] {
      log(rsp_err_);
      co_await close(false);
      co_return;
    }
#ifdef UNIT_TEST_INJECT
    if (g_action == inject_action::close_socket_after_send_length) {
      easylog::warn(
          "inject action: close_socket_after_send_length, conn_id {}, "
          "client_id {}",
          conn_id_, client_id_);
      // Attention: close ssl stream after read error
      // otherwise, server will crash
      co_await close(false);
      co_return;
    }
#endif
  }

  async_simple::coro::Lazy<void> close(bool close_ssl = true) {
    // auto self = shared_from_this();
#ifdef ENABLE_SSL
    if (close_ssl) {
      close_ssl_stream();
    }
#endif
    if (has_closed_) {
      co_return;
    }
    close_socket();
    has_closed_ = true;
  }

#ifdef ENABLE_SSL
  void close_ssl_stream() {
    if (ssl_stream_) {
      asio::error_code ec;
      ssl_stream_->shutdown(ec);
      ssl_stream_ = nullptr;
    }
  }
#endif

  void reset_timer() {
    if (!enable_check_timeout_) {
      return;
    }

    timer_.expires_from_now(keep_alive_timeout_duration_);
    timer_.async_wait(
        [this, self = shared_from_this()](asio::error_code const &ec) {
          if (ec) {
            return;
          }

#ifdef UNIT_TEST_INJECT
          easylog::info("close timeout client_id {} conn_id {}", client_id_,
                        conn_id_);
#else
          easylog::info("close timeout client conn_id {}", conn_id_);
#endif

          close_socket(false);
        });
  }

  void close_socket(bool close_ssl = true) {
    if (has_closed_) {
      return;
    }

    asio::error_code ignored_ec;
    socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ignored_ec);
    socket_.close(ignored_ec);
    has_closed_ = true;
  }

  void cancel_timer() {
    if (!enable_check_timeout_) {
      return;
    }

    asio::error_code ec;
    timer_.cancel(ec);
  }

  asio::io_context &io_context_;
  asio_util::AsioExecutor executor_;
  asio::ip::tcp::socket socket_;
  size_t body_size_ = 256;
  std::vector<char> body_;
  // FIXME: queue's performance can be imporved.
  std::deque<std::vector<char>> write_queue_;
  std::errc rsp_err_;
  bool delay_ = false;

  // if don't get any message in keep_alive_timeout_duration_, the connection
  // will be closed when enable_check_timeout_ is true.
  std::chrono::steady_clock::duration keep_alive_timeout_duration_;
  bool enable_check_timeout_ = false;
  asio::steady_timer timer_;
  std::atomic<bool> has_closed_ = false;
  std::promise<void> promise_;

  QuitCallback quit_callback_ = nullptr;
  uint64_t conn_id_ = 0;

  std::any tag_;
#ifdef ENABLE_SSL
  std::unique_ptr<asio::ssl::stream<asio::ip::tcp::socket &>> ssl_stream_ =
      nullptr;
  bool use_ssl_ = false;
#endif
#ifdef UNIT_TEST_INJECT
  uint32_t client_id_ = 0;
#endif
};
}  // namespace coro_rpc
