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
#include <asio.hpp>
#include <atomic>
#include <deque>
#include <future>
#include <memory>
#include <thread>
#include <type_traits>
#include <variant>
#include <vector>

#include "asio_util/asio_coro_util.hpp"
#include "logging/easylog.hpp"
#include "router.hpp"
#include "router_impl.hpp"
#include "rpc_protocol.h"
#include "struct_pack/struct_pack.hpp"
#ifdef UNIT_TEST_INJECT
#include "inject_action.hpp"
#endif
using asio::ip::tcp;

namespace coro_rpc {
/*!
 * TODO: add doc
 */
class async_connection : public std::enable_shared_from_this<async_connection> {
 public:
  async_connection(std::shared_ptr<asio::io_context> io_context)
      : io_context_(io_context),
        socket_(*io_context),
        body_(body_size_),
        timer_(*io_context) {}

  async_connection(std::shared_ptr<asio::io_context> io_context,
                   std::chrono::steady_clock::duration timeout_duration)
      : async_connection(io_context) {
    if (timeout_duration == std::chrono::seconds(0)) {
      return;
    }

    enable_check_timeout_ = true;
    keep_alive_timeout_duration_ = timeout_duration;
  }

  void start() {
#ifdef ENABLE_SSL
    if (use_ssl_) {
      async_handshake();
    }
    else {
#endif
      read_head();
#ifdef ENABLE_SSL
    }
#endif
  }

  tcp::socket &socket() { return socket_; }

#ifdef ENABLE_SSL
  void init_ssl(asio::ssl::context &ssl_ctx) {
    ssl_stream_ = std::make_unique<asio::ssl::stream<asio::ip::tcp::socket &>>(
        socket_, ssl_ctx);
    use_ssl_ = true;
  }
#endif

#ifdef ENABLE_SSL
  void async_handshake() {
    auto self = this->shared_from_this();
    auto callback = [this, self](const asio::error_code &error) {
      if (error) {
        easylog::error("handshake error: {}", error.message());
        close();
        return;
      }
      read_head();
    };
    ssl_stream_->async_handshake(asio::ssl::stream_base::server, callback);
  }
#endif
  ~async_connection() {
    cancel_timer();
    sync_close();
  }

  void quit() {
    cancel_timer();
    close();

    std::error_code ec;
    io_context_->poll(ec);
    quit_promise_.get_future().wait();
  }

  template <typename R>
  void response_msg(const R &ret) {
    if (has_closed()) [[unlikely]] {
      easylog::info("response_msg failed: connection has been closed");
      return;
    }

    if (io_context_->stopped()) {
      easylog::warn("io_context has been closed");
      return;
    }

    auto buf = struct_pack::serialize_with_offset(RESPONSE_HEADER_LEN, ret);
    *((uint32_t *)buf.data()) = buf.size() - RESPONSE_HEADER_LEN;

    io_context_->post([this, buf = std::move(buf), self = shared_from_this()] {
      if (has_closed()) [[unlikely]] {
        easylog::info("response_msg failed: connection has been closed");
        return;
      }
      write(std::move(buf));
    });
  }

  void response_msg() { response_msg(std::monostate{}); }

  void set_delay(bool b) { delay_ = b; }

  bool has_closed() const { return has_closed_; }

  template <typename T>
  void set_tag(T &&tag) {
    tag_ = std::forward<T>(tag);
  }

  std::any get_tag() { return tag_; }

 private:
  void read_head() {
    async_read_head([this, self = shared_from_this()](asio::error_code ec,
                                                      std::size_t length) {
      if (!ec) {
        assert(length == RPC_HEAD_LEN);
        rpc_header header{};
        auto errc = struct_pack::deserialize_to(header, head_, RPC_HEAD_LEN);
        if (errc != err_ok) [[unlikely]] {
          easylog::info("bad head:{}", struct_pack::error_message(errc));
          close();
          return;
        }

        if (header.magic != magic_number) [[unlikely]] {
          easylog::error("bad magic number");
          close();
          return;
        }

        if (header.length == 0) [[unlikely]] {
          easylog::info("bad body length:{}", header.length);
          close();
          return;
        }

        if (header.length > body_size_) {
          body_size_ = header.length;
          body_.resize(body_size_);
        }
        read_body(header.length);
      }
      else {
        close();
      }
    });
    reset_timer();
  }

  void read_body(std::size_t size) {
    async_read(size, [this, self = shared_from_this()](asio::error_code ec,
                                                       std::size_t length) {
      cancel_timer();
      if (!ec) {
        read_head();
        handle_body(length, self);
      }
      else {
        close(false);
      }
    });
  }

  void handle_body(size_t length, auto &self) {
    auto handler = internal::get_handler({body_.data(), length});
    auto [err, buf] = internal::route(handler, {body_.data(), length}, self);
    if (delay_) {
      delay_ = false;
      return;
    }

    // set buf len at the head.
    *((uint32_t *)buf.data()) = buf.size() - RESPONSE_HEADER_LEN;

    write(std::move(buf));

    if (err != err_ok) [[unlikely]] {
      close(false);
    }
  }

  void write(auto &&buf) {
    write_queue_.push_back(std::move(buf));
    if (write_queue_.size() > 1) {
      return;
    }

    write();
  }

  void write() {
    auto &msg = write_queue_.front();
#ifdef UNIT_TEST_INJECT
    if (g_action == inject_action::force_inject_connection_close_socket) {
      asio::error_code ignored_ec;
      socket_.shutdown(asio::ip::tcp::socket::shutdown_both, ignored_ec);
      socket_.close(ignored_ec);
    }
#endif
    async_write(asio::buffer(msg),
                [this, self = shared_from_this()](asio::error_code ec,
                                                  std::size_t length) {
                  if (ec) {
                    close(false);
                  }
                  else {
                    write_queue_.pop_front();

                    if (!write_queue_.empty()) {
                      write();
                    }
                  }
                });
  }

  template <typename Handler>
  void async_read_head(Handler handler) {
#ifdef ENABLE_SSL
    if (use_ssl_) {
      asio::async_read(*ssl_stream_, asio::buffer(head_, RPC_HEAD_LEN),
                       std::move(handler));
    }
    else {
#endif
      asio::async_read(socket_, asio::buffer(head_, RPC_HEAD_LEN),
                       std::move(handler));
#ifdef ENABLE_SSL
    }
#endif
  }

  template <typename Handler>
  void async_read(size_t size_to_read, Handler handler) {
#ifdef ENABLE_SSL
    if (use_ssl_) {
      asio::async_read(*ssl_stream_, asio::buffer(body_.data(), size_to_read),
                       std::move(handler));
    }
    else {
#endif
      asio::async_read(socket_, asio::buffer(body_.data(), size_to_read),
                       std::move(handler));
#ifdef ENABLE_SSL
    }
#endif
  }

  template <typename BufferType, typename Handler>
  void async_write(const BufferType &buffers, Handler handler) {
#ifdef ENABLE_SSL
    if (use_ssl_) {
      asio::async_write(*ssl_stream_, buffers, std::move(handler));
    }
    else {
#endif
      asio::async_write(socket_, buffers, std::move(handler));
#ifdef ENABLE_SSL
    }
#endif
  }

  void close(bool close_ssl = true) {
    io_context_->post([this, close_ssl, self = shared_from_this()] {
      sync_close(close_ssl);
    });
  }

  void sync_close(bool close_ssl = true) {
    std::call_once(flag_, [this]() {
      quit_promise_.set_value();
    });

#ifdef ENABLE_SSL
    if (!socket_.is_open()) {
      close_ssl_stream();
      return;
    }

    if (use_ssl_ && close_ssl) {
      close_ssl_stream();
    }
#endif

    if (has_closed_) {
      return;
    }

    asio::error_code ignored_ec;
    socket_.shutdown(tcp::socket::shutdown_both, ignored_ec);
    socket_.close(ignored_ec);
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

          close(false);
        });
  }

  void cancel_timer() {
    if (!enable_check_timeout_) {
      return;
    }

    asio::error_code ec;
    timer_.cancel(ec);
  }

  std::shared_ptr<asio::io_context> io_context_;
  tcp::socket socket_;
  char head_[RPC_HEAD_LEN];

  size_t body_size_ = 256;
  std::vector<char> body_;

  std::deque<std::vector<char>> write_queue_;

  bool delay_ = false;

  // if don't get any message in keep_alive_timeout_duration_, the connection
  // will be closed when enable_check_timeout_ is true.
  std::chrono::steady_clock::duration keep_alive_timeout_duration_;
  bool enable_check_timeout_ = false;

  asio::steady_timer timer_;

  std::promise<void> quit_promise_;
  std::atomic_bool has_closed_ = false;
  std::once_flag flag_{};
  std::any tag_;

#ifdef ENABLE_SSL
  std::unique_ptr<asio::ssl::stream<asio::ip::tcp::socket &>> ssl_stream_ =
      nullptr;
  bool use_ssl_ = false;
#endif
};
}  // namespace coro_rpc
