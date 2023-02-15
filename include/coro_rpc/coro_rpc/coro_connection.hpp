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
#include <array>
#include <atomic>
#include <cstdint>
#include <future>
#include <memory>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>
#include <vector>

#include "asio/buffer.hpp"
#include "asio_util/asio_coro_util.hpp"
#include "asio_util/asio_util.hpp"
#include "async_simple/coro/SyncAwait.h"
#include "coro_rpc/coro_rpc/protocol/coro_rpc_protocol.hpp"
#include "easylog/easylog.h"
#ifdef UNIT_TEST_INJECT
#include "inject_action.hpp"
#endif
namespace coro_rpc {
class coro_connection;
using rpc_conn = std::shared_ptr<coro_connection>;
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
        resp_err_(std::errc{}),
        timer_(io_context) {
    body_.resize(body_default_size_);
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
      close();
    }
  }

#ifdef ENABLE_SSL
  void init_ssl(asio::ssl::context &ssl_ctx) {
    ssl_stream_ = std::make_unique<asio::ssl::stream<asio::ip::tcp::socket &>>(
        socket_, ssl_ctx);
    use_ssl_ = true;
  }
#endif

  template <typename rpc_protocol>
  async_simple::coro::Lazy<void> start(
      typename rpc_protocol::router &router) noexcept {
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
        close();
      }
      else {
        ELOGV(INFO, "handshake ok conn_id %d", conn_id_);
        co_await start_impl<rpc_protocol>(router, *ssl_stream_);
      }
    }
    else {
#endif
      co_await start_impl<rpc_protocol>(router, socket_);
#ifdef ENABLE_SSL
    }
#endif
  }
  template <typename rpc_protocol, typename Socket>
  async_simple::coro::Lazy<void> start_impl(
      typename rpc_protocol::router &router, Socket &socket) noexcept {
    while (true) {
      typename rpc_protocol::req_header req_head;
      reset_timer();
      auto ec = co_await rpc_protocol::read_head(socket, req_head);
      cancel_timer();
      // `co_await async_read` uses asio::async_read underlying.
      // If eof occurred, the bytes_transferred of `co_await async_read` must
      // less than RPC_HEAD_LEN. Incomplete data will be discarded.
      // So, no special handling of eof is required.
      if (ec) {
        ELOGV(ERROR, "%s, %s", ec.message().data(), "read head error");
        close();
        co_return;
      }

#ifdef UNIT_TEST_INJECT
      client_id_ = req_head.seq_num;
      ELOGV(INFO, "conn_id %d, client_id %d", conn_id_, client_id_);
#endif

#ifdef UNIT_TEST_INJECT
      if (g_action == inject_action::close_socket_after_read_header) {
        ELOGV(WARN,
              "inject action: close_socket_after_read_header, conn_id %d, "
              "client_id %d",
              conn_id_, client_id_);
        close();
        co_return;
      }
#endif
      auto serialize_proto = rpc_protocol::get_serialize_protocol(req_head);

      if (!serialize_proto.has_value()) [[unlikely]] {
        ELOGV(ERROR, "bad serialize protocol type, conn_id %d", conn_id_);
        close();
        co_return;
      }

      // read payload
      ec = co_await rpc_protocol::read_payload(socket, req_head, body_);
      if (ec) [[unlikely]] {
        ELOGV(ERROR, "read error: %s, conn_id %d", ec.message().data(),
              conn_id_);
        close();
        co_return;
      }

      std::pair<std::errc, std::string> pair{};

      auto payload = std::string_view{body_.data(), body_.size()};

      auto key = rpc_protocol::get_route_key(req_head);
      auto handler = router.get_handler(key);
      if (!handler) {
        auto coro_handler = router.get_coro_handler(key);
        pair = co_await router.route_coro(coro_handler, payload,
                                          shared_from_this(), req_head,
                                          serialize_proto.value());
      }
      else {
        pair = router.route(handler, payload, shared_from_this(), req_head,
                            serialize_proto.value());
      }

      auto &[resp_err, resp_buf] = pair;
      if (delay_) {
        delay_ = false;
        continue;
      }

      std::string header_buf =
          rpc_protocol::write_head(resp_buf, req_head, resp_err);

#ifdef UNIT_TEST_INJECT
      if (g_action == inject_action::close_socket_after_send_length) {
        ELOGV(WARN,
              "inject action: close_socket_after_send_length conn_id %d, "
              "client_id %d",
              conn_id_, client_id_);
        co_await asio_util::async_write(socket, asio::buffer(header_buf));
        close();
        co_return;
      }
      if (g_action == inject_action::server_send_bad_rpc_result) {
        ELOGV(WARN,
              "inject action: server_send_bad_rpc_result conn_id %d, client_id "
              "%d",
              conn_id_, client_id_);
        resp_buf[0] = resp_buf[0] + 1;
      }
#endif
      if (resp_err_ == std::errc{}) [[likely]] {
        if (resp_err != std::errc{}) [[unlikely]] {
          resp_err_ = resp_err;
        }
        write_queue_.emplace_back(std::move(header_buf), std::move(resp_buf));
        if (write_queue_.size() == 1) {
          send_data().start([self = shared_from_this()](auto &&) {
          });
        }
        if (resp_err != std::errc{}) [[unlikely]] {
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

  template <typename rpc_protocol>
  void response_msg(std::string &&body_buf,
                    const typename rpc_protocol::req_header &req_head) {
    std::string header_buf = rpc_protocol::write_head(body_buf, req_head);
    response(std::move(header_buf), std::move(body_buf), shared_from_this())
        .via(&executor_)
        .detach();
  }

  void set_delay(bool b) { delay_ = b; }

  /*!
   * Check the connection has closed or not
   *
   * @return true if closed, otherwise false.
   */
  bool has_closed() const { return has_closed_; }

  /*!
   * close connection asynchronously
   *
   * If the connection has already closed, nothing will happen.
   *
   * @param close_ssl whether to close the ssl stream
   */
  void async_close() {
    if (has_closed_) {
      return;
    }
    asio::post(io_context_, [this, self = shared_from_this()] {
      this->close();
    });
  }

  void close_coro() {}

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
                                          rpc_conn self) noexcept {
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
        close();
        co_return;
      }
#endif
      std::array<asio::const_buffer, 2> buffers{asio::buffer(msg.first),
                                                asio::buffer(msg.second)};
#ifdef ENABLE_SSL
      if (use_ssl_) {
        assert(ssl_stream_);
        ret = co_await asio_util::async_write(*ssl_stream_, buffers);
      }
      else {
#endif
        ret = co_await asio_util::async_write(socket_, buffers);
#ifdef ENABLE_SSL
      }
#endif
      if (ret.first) [[unlikely]] {
        ELOGV(ERROR, "%s, %s", ret.first.message().data(), "async_write error");
        close();
        co_return;
      }
      write_queue_.pop_front();
    }
    if (resp_err_ != std::errc{}) [[unlikely]] {
      ELOGV(ERROR, "%s, %s", std::make_error_code(resp_err_).message().data(),
            "resp_err_");
      close();
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
      close();
      co_return;
    }
#endif
  }

  void close() {
    if (has_closed_) {
      return;
    }
    close_socket();
    if (quit_callback_) {
      quit_callback_(conn_id_);
    }
    has_closed_ = true;
  }

  void reset_timer() {
    if (!enable_check_timeout_) {
      return;
    }

    timer_.expires_from_now(keep_alive_timeout_duration_);
    timer_.async_wait(
        [this, self = shared_from_this()](asio::error_code const &ec) {
          if (!ec) {
#ifdef UNIT_TEST_INJECT
            ELOGV(INFO, "close timeout client_id %d conn_id %d", client_id_,
                  conn_id_);
#else
            ELOGV(INFO, "close timeout client conn_id %d", conn_id_);
#endif

            close_socket();
          }
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
  constexpr static size_t body_default_size_ = 256;
  std::vector<char> body_;
  // FIXME: queue's performance can be imporved.
  std::deque<std::pair<std::string, std::string>> write_queue_;
  std::errc resp_err_;
  bool delay_ = false;

  // if don't get any message in keep_alive_timeout_duration_, the connection
  // will be closed when enable_check_timeout_ is true.
  std::chrono::steady_clock::duration keep_alive_timeout_duration_;
  bool enable_check_timeout_ = false;
  asio::steady_timer timer_;
  std::atomic<bool> has_closed_ = false;

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
