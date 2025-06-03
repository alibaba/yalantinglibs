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
#ifdef YLT_ENABLE_IBV
#include "ibverbs/ib_io.hpp"
#include "ibverbs/ib_socket.hpp"
#endif
#include "io_context_pool.hpp"
namespace coro_io {
struct socket_wrapper_t {
  // construct by listen tcp
  socket_wrapper_t() {};
  socket_wrapper_t(coro_io::ExecutorWrapper<> *executor,
                   const std::string &local_ip = "")
      : executor_(executor) {
    if (local_ip.empty()) {
      socket_ = std::make_unique<asio::ip::tcp::socket>(
          executor->get_asio_executor());
    }
    else {
      asio::error_code ec;
      socket_ = std::make_unique<asio::ip::tcp::socket>(
          executor->get_asio_executor(),
          asio::ip::tcp::endpoint(asio::ip::address::from_string(local_ip, ec),
                                  0));
    }

    init_client(true);
  };
  socket_wrapper_t(asio::ip::tcp::socket &&soc,
                   coro_io::ExecutorWrapper<> *executor)
      : socket_(std::make_unique<asio::ip::tcp::socket>(std::move(soc))),
        executor_(executor) {}
#ifdef YLT_ENABLE_SSL
  // construct by listen tcp & make ssl
  socket_wrapper_t(asio::ip::tcp::socket &&soc,
                   coro_io::ExecutorWrapper<> *executor,
                   asio::ssl::context &ssl_ctx)
      : socket_(std::make_unique<asio::ip::tcp::socket>(std::move(soc))),
        executor_(executor) {
    init_ssl(ssl_ctx);
  }
#endif
#ifdef YLT_ENABLE_IBV
  socket_wrapper_t(asio::ip::tcp::socket &&soc,
                   coro_io::ExecutorWrapper<> *executor,
                   const coro_io::ibverbs_config &config,
                   std::shared_ptr<coro_io::ib_device_t> dev,
                   std::shared_ptr<coro_io::ib_buffer_pool_t> buffer_pool)
      : executor_(executor),
        ib_socket_(std::make_unique<ib_socket_t>(executor_, dev, config,
                                                 buffer_pool)) {
    ib_socket_->prepare_accpet(std::move(soc));
  }
#endif
  // tcp client init
  void init_client(bool enable_tcp_no_delay) {
    std::error_code ec;
    socket_->set_option(asio::ip::tcp::no_delay(enable_tcp_no_delay), ec);
  }
#ifdef YLT_ENABLE_SSL
  // ssl client init
  void init_client(asio::ssl::context &ssl_ctx, bool enable_tcp_no_delay) {
    std::error_code ec;
    socket_->set_option(asio::ip::tcp::no_delay(enable_tcp_no_delay), ec);
    init_ssl(ssl_ctx);
  }
#endif
#ifdef YLT_ENABLE_IBV
  void init_client(const coro_io::ibverbs_config &config,
                   std::shared_ptr<coro_io::ib_device_t> device,
                   std::shared_ptr<coro_io::ib_buffer_pool_t> buffer_pool) {
    if (ib_socket_) {
      *ib_socket_ = ib_socket_t(executor_, std::move(device), config,
                                std::move(buffer_pool));
    }
    else {
      ib_socket_ = std::make_unique<ib_socket_t>(
          executor_, std::move(device), config, std::move(buffer_pool));
    }
  }
#endif

 private:
  std::unique_ptr<asio::ip::tcp::socket> socket_;
  coro_io::ExecutorWrapper<> *executor_;
#ifdef YLT_ENABLE_SSL
  std::unique_ptr<asio::ssl::stream<asio::ip::tcp::socket &>> ssl_stream_;
#endif

#ifdef YLT_ENABLE_IBV
  std::unique_ptr<ib_socket_t> ib_socket_;
#endif

 public:
  bool use_ssl() const noexcept {
#ifdef YLT_ENABLE_SSL
    return ssl_stream_ != nullptr;
#else
    return false;
#endif
  }
  template <typename T>
  auto visit_coro(T &&op) noexcept {
#ifdef YLT_ENABLE_IBV
    if (ib_socket_) {
      return op(*ib_socket_);
    }
#endif
#ifdef YLT_ENABLE_SSL
    if (use_ssl()) {
      return op(*ssl_stream_);
    }
#endif
    return op(*socket_);
  }
  template <typename T>
  auto visit(T &&op) noexcept {
#ifdef YLT_ENABLE_IBV
    if (ib_socket_) {
      return op(*ib_socket_);
    }
#endif
#ifdef YLT_ENABLE_SSL
    if (use_ssl()) {
      return op(*ssl_stream_);
    }
#endif
    return op(*socket_);
  }

  void close() noexcept {
    std::error_code ignored_ec;
#ifdef YLT_ENABLE_IBV
    if (ib_socket_) {
      ib_socket_->close();
      return;
    }
#endif
    if (socket_) {
      socket_->shutdown(asio::ip::tcp::socket::shutdown_both, ignored_ec);
      socket_->close(ignored_ec);
    }
  }

  coro_io::endpoint remote_endpoint() {
#ifdef YLT_ENABLE_IBV
    if (ib_socket_) {
      return {ib_socket_->get_remote_address(), ib_socket_->get_remote_qp_num(),
              coro_io::endpoint::rdma};
    }
#endif
    return {socket_->remote_endpoint().address(),
            socket_->remote_endpoint().port(), coro_io::endpoint::tcp};
  }
  coro_io::endpoint local_endpoint() {
#ifdef YLT_ENABLE_IBV
    if (ib_socket_) {
      return {ib_socket_->get_local_address(), ib_socket_->get_local_qp_num(),
              coro_io::endpoint::rdma};
    }
#endif
    return {socket_->local_endpoint().address(),
            socket_->local_endpoint().port(), coro_io::endpoint::tcp};
  }

  auto get_executor() const noexcept { return executor_; }
  std::unique_ptr<asio::ip::tcp::socket> &socket() noexcept { return socket_; }
  using tcp_socket_t = asio::ip::tcp::socket;
#ifdef YLT_ENABLE_SSL
  void init_ssl(asio::ssl::context &ssl_ctx) {
    ssl_stream_ = std::make_unique<asio::ssl::stream<asio::ip::tcp::socket &>>(
        *socket_, ssl_ctx);
  }
  std::unique_ptr<asio::ssl::stream<asio::ip::tcp::socket &>> &
  ssl_stream() noexcept {
    return ssl_stream_;
  }
  using tcp_socket_with_ssl_t = asio::ssl::stream<asio::ip::tcp::socket &>;
#endif
#ifdef YLT_ENABLE_IBV
  using ibv_socket_t = coro_io::ib_socket_t;
#endif
};
}  // namespace coro_io