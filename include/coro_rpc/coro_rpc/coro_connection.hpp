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
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>
#include <vector>

#include "asio_util/asio_coro_util.hpp"
#include "asio_util/asio_util.hpp"
#include "async_simple/coro/SyncAwait.h"
#include "easylog/easylog.h"
#include "router.hpp"
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
      ELOGV(INFO, "~async_connection conn_id %d, client_id %d", conn_id_,
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

  template <typename server_config>
  async_simple::coro::Lazy<void> start(
      internal::router<server_config> &router) noexcept {
#ifdef ENABLE_SSL
    if (use_ssl_) {
      assert(ssl_stream_);
      ELOGV(INFO, "begin to handshake conn_id %d", conn_id_);
      reset_timer();
      auto shake_ec = co_await asio_util::async_handshake(
          ssl_stream_, asio::ssl::stream_base::server);
      cancel_timer();
      if (shake_ec) {
        ELOGV(ERROR, "handshake failed: %s conn_id %d",
              shake_ec.message().data(), conn_id_);
        co_await close();
        co_return;
      }
      ELOGV(INFO, "handshake ok conn_id %d", conn_id_);
      co_await start_impl(router, *ssl_stream_);
    }
    else {
#endif
      co_await start_impl(router, socket_);
#ifdef ENABLE_SSL
    }
#endif
    if (quit_callback_) {
      quit_callback_(conn_id_);
    }
    promise_.set_value();
  }
  template <typename server_config, typename Socket>
  async_simple::coro::Lazy<void> start_impl(
      internal::router<server_config> &router, Socket &socket) noexcept {
    char head_[RPC_HEAD_LEN];
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
        ELOGV(ERROR, "%s, %s", ret.first.message().data(), "read head error");
        co_await close();
        ELOGV(ERROR, "hello");
        co_return;
      }
      assert(ret.second == RPC_HEAD_LEN);
      auto errc = struct_pack::deserialize_to(header_, head_, RPC_HEAD_LEN);
      if (errc != struct_pack::errc::ok) [[unlikely]] {
        ELOGV(ERROR, "%s, %s",
              std::make_error_code(std::errc::protocol_error).message().data(),
              "deserialize error");
        co_await close();
        co_return;
      }

#ifdef UNIT_TEST_INJECT
      client_id_ = header_.seq_num;
      ELOGV(INFO, "conn_id %d, client_id %d", conn_id_, client_id_);
#endif

#ifdef UNIT_TEST_INJECT
      if (g_action == inject_action::close_socket_after_read_header) {
        ELOGV(WARN,
              "inject action: close_socket_after_read_header, conn_id %d, "
              "client_id %d",
              conn_id_, client_id_);
        co_await close();
        co_return;
      }
#endif
      if (header_.magic != magic_number) [[unlikely]] {
        ELOGV(ERROR, "bad magic number, conn_id %d", conn_id_);
        co_await close();
        co_return;
      }
      if (header_.length < 4) [[unlikely]] {
        ELOGV(ERROR, "bad length: %d, conn_id %d", header_.length, conn_id_);
        co_await close();
        co_return;
      }

      if (header_.length > body_size_) {
        body_size_ = header_.length;
        body_.resize(body_size_);
      }

      ret = co_await asio_util::async_read(
          socket, asio::buffer(body_.data(), header_.length));
      if (ret.first) [[unlikely]] {
        ELOGV(ERROR, "read error: %s, conn_id %d", ret.first.message().data(),
              conn_id_);
        co_await close();
        co_return;
      }

      std::pair<std::errc, std::string> pair{};

      uint32_t function_id = *(uint32_t *)body_.data();

      auto payload = std::string_view{body_.data() + 4, ret.second - 4};

      auto handler = router.get_handler(function_id);
      if (!handler) {
        auto coro_handler = router.get_coro_handler(function_id);
        pair = co_await router.route_coro(function_id, coro_handler, payload,
                                          self);
      }
      else {
        pair = router.route(function_id, handler, payload, self);
      }

      auto &[err, body_buf] = pair;
      if (delay_) {
        delay_ = false;
        continue;
      }

      rpc_header resp_header = header_;
      resp_header.length = body_buf.size();
      auto header_buf = struct_pack::serialize<std::string>(resp_header);

#ifdef UNIT_TEST_INJECT
      if (g_action == inject_action::close_socket_after_send_length) {
        ELOGV(WARN,
              "inject action: close_socket_after_send_length conn_id %d, "
              "client_id %d",
              conn_id_, client_id_);
        co_await asio_util::async_write(socket, asio::buffer(header_buf));
        co_await close();
        co_return;
      }
      if (g_action == inject_action::server_send_bad_rpc_result) {
        ELOGV(WARN,
              "inject action: server_send_bad_rpc_result conn_id %d, client_id "
              "%d",
              conn_id_, client_id_);
        body_buf[0] = body_buf[0] + 1;
      }
#endif
      if (rsp_err_ == err_ok) [[likely]] {
        if (err != err_ok) [[unlikely]] {
          rsp_err_ = err;
        }
        write_queue_.emplace_back(std::move(header_buf), std::move(body_buf));
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
      ELOGV(DEBUG, "response_msg failed: connection has been closed");
      return;
    }

    auto body_buf = struct_pack::serialize<std::string>(ret);
    rpc_header resp_header = header_;
    resp_header.length = body_buf.size();
    auto header_buf = struct_pack::serialize<std::string>(resp_header);

    auto self = shared_from_this();
    response(std::move(header_buf), std::move(body_buf), std::move(self))
        .via(&executor_)
        .detach();
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
  void sync_close() {
    if (has_closed_) {
      return;
    }

#ifdef UNIT_TEST_INJECT
    ELOGV(INFO, "sync_close conn_id %d, client_id %d", conn_id_, client_id_);
#else
    ELOGV(INFO, "sync_close conn_id %d", conn_id_);
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
  async_simple::coro::Lazy<void> response(std::string header_buf,
                                          std::string body_buf,
                                          auto self) noexcept {
    if (has_closed()) [[unlikely]] {
      ELOGV(DEBUG, "response_msg failed: connection has been closed");
      co_return;
    }
#ifdef UNIT_TEST_INJECT
    if (g_action == inject_action::close_socket_after_send_length) {
      ELOGV(WARN, "inject action: close_socket_after_send_length");
      body_buf.clear();
    }
#endif
    write_queue_.emplace_back(std::move(header_buf), std::move(body_buf));
    if (write_queue_.size() > 1) {
      co_return;
    }
    co_await send_data();
  }

  async_simple::coro::Lazy<void> send_data() {
    std::pair<std::error_code, size_t> ret;
    while (!write_queue_.empty()) {
      auto &msg = write_queue_.front();
#ifdef UNIT_TEST_INJECT
      if (g_action == inject_action::force_inject_connection_close_socket) {
        ELOGV(
            WARN,
            "inject action: force_inject_connection_close_socket, conn_id %d, "
            "client_id %d",
            conn_id_, client_id_);
        co_await close();
        co_return;
      }
#endif
#ifdef ENABLE_SSL
      if (use_ssl_) {
        assert(ssl_stream_);
        ret = co_await asio_util::async_write(*ssl_stream_,
                                              asio::buffer(msg.first));
        if (!ret.first) [[likely]] {
          ret = co_await asio_util::async_write(*ssl_stream_,
                                                asio::buffer(msg.second));
        }
      }
      else {
#endif
        ret = co_await asio_util::async_write(socket_, asio::buffer(msg.first));
        if (!ret.first) [[likely]] {
          ret = co_await asio_util::async_write(socket_,
                                                asio::buffer(msg.second));
        }
#ifdef ENABLE_SSL
      }
#endif
      if (ret.first) [[unlikely]] {
        ELOGV(ERROR, "%s, %s", ret.first.message().data(), "async_write error");
        co_await close();
        co_return;
      }
      write_queue_.pop_front();
    }
    if (rsp_err_ != err_ok) [[unlikely]] {
      ELOGV(ERROR, "%s, %s", std::make_error_code(rsp_err_).message().data(),
            "rsp_err_");
      co_await close();
      co_return;
    }
#ifdef UNIT_TEST_INJECT
    if (g_action == inject_action::close_socket_after_send_length) {
      ELOGV(INFO,
            "inject action: close_socket_after_send_length, conn_id %d, "
            "client_id %d",
            conn_id_, client_id_);
      // Attention: close ssl stream after read error
      // otherwise, server will crash
      co_await close();
      co_return;
    }
#endif
  }

  async_simple::coro::Lazy<void> close() {
    if (has_closed_) {
      co_return;
    }
    close_socket();
    has_closed_ = true;
  }

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
          ELOGV(INFO, "close timeout client_id %d conn_id %d", client_id_,
                conn_id_);
#else
          ELOGV(INFO, "close timeout client conn_id %d", conn_id_);
#endif

          close_socket();
        });
  }

  void close_socket() {
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
  std::deque<std::pair<std::string, std::string>> write_queue_;
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
  rpc_header header_{};

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
