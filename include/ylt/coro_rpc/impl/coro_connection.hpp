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
#include <async_simple/coro/SyncAwait.h>

#include <any>
#include <array>
#include <asio/buffer.hpp>
#include <atomic>
#include <cstdint>
#include <future>
#include <memory>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>
#include <vector>
#include <ylt/easylog.hpp>

#include "ylt/coro_io/coro_io.hpp"
#ifdef UNIT_TEST_INJECT
#include "inject_action.hpp"
#endif
namespace coro_rpc {

template <typename T>
concept apply_user_buf = requires() {
  requires std::is_same_v<std::string_view,
                          typename std::remove_cvref_t<T>::buffer_type>;
};

class coro_connection;
using rpc_conn = std::shared_ptr<coro_connection>;

template <typename rpc_protocol>
struct context_info_t {
  constexpr static size_t body_default_size_ = 256;
  std::shared_ptr<coro_connection> conn_;
  typename rpc_protocol::req_header req_head_;
  std::vector<char> body_;
  std::atomic<bool> has_response_ = false;
  bool is_delay_ = false;
  context_info_t(std::shared_ptr<coro_connection> &&conn)
      : conn_(std::move(conn)) {}
};
/*!
 * TODO: add doc
 */

[[noreturn]] inline void unreachable() {
  // Uses compiler specific extensions if possible.
  // Even if no extension is used, undefined behavior is still raised by
  // an empty function body and the noreturn attribute.
#ifdef __GNUC__  // GCC, Clang, ICC
  __builtin_unreachable();
#elif defined(_MSC_VER)  // msvc
  __assume(false);
#endif
}

class coro_connection : public std::enable_shared_from_this<coro_connection> {
 public:
  enum rpc_call_type {
    non_callback,
    callback_with_delay,
    callback_finished,
    callback_started
  };

  /*!
   *
   * @param io_context
   * @param socket
   * @param timeout_duration
   */
  template <typename executor_t>
  coro_connection(executor_t *executor, asio::ip::tcp::socket socket,
                  std::chrono::steady_clock::duration timeout_duration =
                      std::chrono::seconds(0))
      : executor_(executor),
        socket_(std::move(socket)),
        resp_err_(std::errc{}),
        timer_(executor->get_asio_executor()) {
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

#ifdef YLT_ENABLE_SSL
  void init_ssl(asio::ssl::context &ssl_ctx) {
    ssl_stream_ = std::make_unique<asio::ssl::stream<asio::ip::tcp::socket &>>(
        socket_, ssl_ctx);
    use_ssl_ = true;
  }
#endif

  template <typename rpc_protocol>
  async_simple::coro::Lazy<void> start(
      typename rpc_protocol::router &router) noexcept {
#ifdef YLT_ENABLE_SSL
    if (use_ssl_) {
      assert(ssl_stream_);
      ELOGV(INFO, "begin to handshake conn_id %d", conn_id_);
      reset_timer();
      auto shake_ec = co_await coro_io::async_handshake(
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
#ifdef YLT_ENABLE_SSL
    }
#endif
  }
  template <typename rpc_protocol, typename Socket>
  async_simple::coro::Lazy<void> start_impl(
      typename rpc_protocol::router &router, Socket &socket) noexcept {
    auto context_info =
        std::make_shared<context_info_t<rpc_protocol>>(shared_from_this());
    std::string resp_error_msg;
    while (true) {
      auto &req_head = context_info->req_head_;
      auto &body = context_info->body_;
      reset_timer();
      auto ec = co_await rpc_protocol::read_head(socket, req_head);
      cancel_timer();
      // `co_await async_read` uses asio::async_read underlying.
      // If eof occurred, the bytes_transferred of `co_await async_read` must
      // less than RPC_HEAD_LEN. Incomplete data will be discarded.
      // So, no special handling of eof is required.
      if (ec) {
        ELOGV(INFO, "connection %d close: %s", conn_id_, ec.message().data());
        close();
        break;
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
        break;
      }
#endif
      auto serialize_proto = rpc_protocol::get_serialize_protocol(req_head);

      if (!serialize_proto.has_value())
        AS_UNLIKELY {
          ELOGV(ERROR, "bad serialize protocol type, conn_id %d", conn_id_);
          close();
          break;
        }

      std::string_view payload{};
      // rpc_protocol::buffer_type maybe from user, default from framework.
      constexpr bool apply_user_buf_v = apply_user_buf<rpc_protocol>;
      if constexpr (apply_user_buf_v) {
        ec = co_await rpc_protocol::read_payload(socket, req_head, payload);
      }
      else {
        ec = co_await rpc_protocol::read_payload(socket, req_head, body);
        payload = std::string_view{body.data(), body.size()};
      }

      if (ec)
        AS_UNLIKELY {
          ELOGV(ERROR, "read error: %s, conn_id %d", ec.message().data(),
                conn_id_);
          close();
          break;
        }

      std::pair<std::errc, std::string> pair{};

      auto key = rpc_protocol::get_route_key(req_head);
      auto handler = router.get_handler(key);
      if (!handler) {
        auto coro_handler = router.get_coro_handler(key);
        pair = co_await router.route_coro(coro_handler, payload, context_info,
                                          serialize_proto.value(), key);
      }
      else {
        pair = router.route(handler, payload, context_info,
                            serialize_proto.value(), key);
      }

      auto &[resp_err, resp_buf] = pair;
      switch (rpc_call_type_) {
        default:
          unreachable();
        case rpc_call_type::non_callback:
          break;
        case rpc_call_type::callback_with_delay:
          ++delay_resp_cnt;
          rpc_call_type_ = rpc_call_type::non_callback;
          context_info = std::make_shared<context_info_t<rpc_protocol>>(
              shared_from_this());
          continue;
        case rpc_call_type::callback_finished:
          continue;
        case rpc_call_type::callback_started:
          coro_io::callback_awaitor<void> awaitor;
          rpc_call_type_ = rpc_call_type::callback_finished;
          co_await awaitor.await_resume([this](auto handler) {
            this->callback_awaitor_handler_ = std::move(handler);
          });
          context_info->has_response_ = false;
          rpc_call_type_ = rpc_call_type::non_callback;
          continue;
      }
      resp_error_msg.clear();
      if (resp_err != std::errc{})
        AS_UNLIKELY { std::swap(resp_buf, resp_error_msg); }
      std::string header_buf = rpc_protocol::prepare_response(
          resp_buf, req_head, resp_err, resp_error_msg);

#ifdef UNIT_TEST_INJECT
      if (g_action == inject_action::close_socket_after_send_length) {
        ELOGV(WARN,
              "inject action: close_socket_after_send_length conn_id %d, "
              "client_id %d",
              conn_id_, client_id_);
        co_await coro_io::async_write(socket, asio::buffer(header_buf));
        close();
        break;
      }
      if (g_action == inject_action::server_send_bad_rpc_result) {
        ELOGV(WARN,
              "inject action: server_send_bad_rpc_result conn_id %d, client_id "
              "%d",
              conn_id_, client_id_);
        resp_buf[0] = resp_buf[0] + 1;
      }
#endif
      if (resp_err_ == std::errc{})
        AS_LIKELY {
          if (resp_err != std::errc{})
            AS_UNLIKELY { resp_err_ = resp_err; }
          write_queue_.emplace_back(std::move(header_buf), std::move(resp_buf));
          if (write_queue_.size() == 1) {
            send_data().start([self = shared_from_this()](auto &&) {
            });
          }
          if (resp_err != std::errc{})
            AS_UNLIKELY { break; }
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
                    const typename rpc_protocol::req_header &req_head,
                    bool is_delay) {
    std::string header_buf = rpc_protocol::prepare_response(body_buf, req_head);
    response(std::move(header_buf), std::move(body_buf), shared_from_this(),
             is_delay)
        .via(executor_)
        .detach();
  }

  void set_rpc_call_type(enum rpc_call_type r) { rpc_call_type_ = r; }

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
    executor_->schedule([this, self = shared_from_this()] {
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

  auto &get_executor() { return *executor_; }

 private:
  async_simple::coro::Lazy<void> response(std::string header_buf,
                                          std::string body_buf, rpc_conn self,
                                          bool is_delay) noexcept {
    if (has_closed())
      AS_UNLIKELY {
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
    if (is_delay) {
      --delay_resp_cnt;
      assert(delay_resp_cnt >= 0);
      reset_timer();
    }
    if (write_queue_.size() == 1) {
      co_await send_data();
    }
    if (!is_delay) {
      if (rpc_call_type_ == rpc_call_type::callback_finished) {
        // the function start_impl is waiting for resume.
        callback_awaitor_handler_.resume();
      }
      else {
        assert(rpc_call_type_ == rpc_call_type::callback_started);
        // the function start_impl is not waiting for resume.
        rpc_call_type_ = rpc_call_type::callback_finished;
      }
    }
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
#ifdef YLT_ENABLE_SSL
      if (use_ssl_) {
        assert(ssl_stream_);
        ret = co_await coro_io::async_write(*ssl_stream_, buffers);
      }
      else {
#endif
        ret = co_await coro_io::async_write(socket_, buffers);
#ifdef YLT_ENABLE_SSL
      }
#endif
      if (ret.first)
        AS_UNLIKELY {
          ELOGV(ERROR, "%s, %s", ret.first.message().data(),
                "async_write error");
          close();
          co_return;
        }
      write_queue_.pop_front();
    }
    if (resp_err_ != std::errc{})
      AS_UNLIKELY {
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
    if (!enable_check_timeout_ || delay_resp_cnt != 0) {
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

  coro_io::callback_awaitor<void>::awaitor_handler callback_awaitor_handler_{
      nullptr};
  async_simple::Executor *executor_;
  asio::ip::tcp::socket socket_;
  // FIXME: queue's performance can be imporved.
  std::deque<std::pair<std::string, std::string>> write_queue_;
  std::errc resp_err_;
  rpc_call_type rpc_call_type_{non_callback};

  // if don't get any message in keep_alive_timeout_duration_, the connection
  // will be closed when enable_check_timeout_ is true.
  std::chrono::steady_clock::duration keep_alive_timeout_duration_;
  bool enable_check_timeout_{false};
  asio::steady_timer timer_;
  std::atomic<bool> has_closed_{false};

  QuitCallback quit_callback_{nullptr};
  uint64_t conn_id_{0};
  uint64_t delay_resp_cnt{0};

  std::any tag_;

#ifdef YLT_ENABLE_SSL
  std::unique_ptr<asio::ssl::stream<asio::ip::tcp::socket &>> ssl_stream_ =
      nullptr;
  bool use_ssl_ = false;
#endif
#ifdef UNIT_TEST_INJECT
  uint32_t client_id_ = 0;
#endif
};

}  // namespace coro_rpc
