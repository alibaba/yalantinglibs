#pragma once
#include <async_simple/Executor.h>
#include <async_simple/coro/Lazy.h>

#include <asio/ip/tcp.hpp>
#include <atomic>
#include <charconv>
#include <cstdint>
#include <optional>
#include <string>
#include <system_error>

#include "listen_endpoint.hpp"
#include "socket_wrapper.hpp"
#include "ylt/coro_io/coro_io.hpp"
#include "ylt/coro_io/io_context_pool.hpp"
#include "ylt/util/expected.hpp"

namespace coro_io {

enum class listen_errc {
  ok,
  bad_address,
  open_error,
  address_in_used,
  listen_error,
  unknown_error
};

struct server_acceptor_base {
  server_acceptor_base(server_acceptor_base&&) = default;
  server_acceptor_base& operator=(server_acceptor_base&&) = default;
  uint16_t port() const noexcept { return port_; }
  std::string_view address() const { return address_; }
  virtual void close() = 0;
  virtual void close_now() = 0;
  virtual listen_errc listen() = 0;
  virtual async_simple::coro::Lazy<
      ylt::expected<coro_io::socket_wrapper_t, std::error_code>>
  accept() = 0;
  virtual ~server_acceptor_base() = default;
  server_acceptor_base(std::string_view address, uint16_t port = 0)
      : port_(port) {
    init_address(address);
  }
  void set_io_threads_pool(coro_io::io_context_pool* pool) noexcept {
    if (pool_ == nullptr)
      pool_ = pool;
  }
  void init_address(std::string_view address) {
    auto parsed = detail::parse_listen_address(address, port_);
    address_ = std::move(parsed.address);
    port_ = parsed.port;
  }
  void set_ipv6_dual_stack(bool v) noexcept { ipv6_dual_stack_ = v; }
  bool ipv6_dual_stack() const noexcept { return ipv6_dual_stack_; }
  uint16_t port_;
  std::string address_;
  coro_io::io_context_pool* pool_ = nullptr;
  bool ipv6_dual_stack_ = false;
};

struct tcp_server_acceptor : public server_acceptor_base {
  virtual listen_errc listen() override {
    executor_ = pool_->get_executor();
    acceptor_ = asio::ip::tcp::acceptor{executor_->get_asio_executor()};
    ELOG_INFO << "begin to listen";
    using asio::ip::tcp;
    asio::error_code ec;

    auto endpoint = detail::resolve_listen_endpoint(
        executor_->get_asio_executor(), address_, port_, ec);
    if (!endpoint) {
      ELOG_ERROR << "resolve address " << address_
                 << " error: " << ec.message();
      return listen_errc::bad_address;
    }

    auto mode = ipv6_dual_stack_ ? detail::ipv6_only_mode::enable
                                 : detail::ipv6_only_mode::disable;
    ec = detail::init_tcp_acceptor(*acceptor_, *endpoint, mode);
    if (ec) {
      ELOG_ERROR << "listen init failed, error: " << ec.message();
      if (ec == asio::error::address_in_use) {
        return listen_errc::address_in_used;
      }
      if (ec == asio::error::invalid_argument ||
          ec == asio::error::address_family_not_supported ||
          ec == asio::error::fault) {
        return listen_errc::bad_address;
      }
      if (ec == asio::error::already_open ||
          ec == asio::error::operation_not_supported ||
          ec == asio::error::no_descriptors) {
        return listen_errc::open_error;
      }
      return listen_errc::listen_error;
    }

    auto end_point = acceptor_->local_endpoint(ec);
    if (ec) {
      ELOG_ERROR << "get local endpoint port " << port_
                 << " error: " << ec.message();
      return listen_errc::address_in_used;
    }
    port_ = end_point.port();

    ELOG_INFO << "listen port " << port_ << " successfully";
    return {};
  }
  virtual async_simple::coro::Lazy<
      ylt::expected<coro_io::socket_wrapper_t, std::error_code>>
  accept() override {
    accept_started_.store(true, std::memory_order_release);
    assert(acceptor_ != std::nullopt);
    auto socket_executor = pool_->get_executor();
    asio::ip::tcp::socket socket(socket_executor->get_asio_executor());
    ELOG_TRACE << "start accepting from acceptor: " << address_ << ":" << port_;
    co_await coro_io::dispatch(executor_->get_asio_executor());
    auto error = co_await coro_io::async_accept(*acceptor_, socket);
    ELOG_TRACE << "get connection from acceptor: " << address_ << ":" << port_;
    if (error) {
      if (error == asio::error::operation_aborted) {
        ELOG_INFO << "accept canceled by server quit: " << error.message();
      }
      else {
        ELOG_ERROR << "accept error: " << error.message();
      }
      if (error == asio::error::operation_aborted ||
          error == asio::error::bad_descriptor) {
        acceptor_close_waiter_.set_value();
      }
      co_return ylt::expected<coro_io::socket_wrapper_t, std::error_code>{
          ylt::unexpected<std::error_code>{error}};
    }
    else {
      co_return coro_io::socket_wrapper_t{std::move(socket), socket_executor};
    }
  }

  virtual void close_now() override {
    if (!acceptor_) {
      return;
    }
    if (!accept_started_.load(std::memory_order_acquire)) {
      detail::close_acceptor_now(*acceptor_);
      return;
    }
    asio::dispatch(executor_->get_asio_executor(), [this]() {
      if (acceptor_) {
        detail::close_acceptor_now(*acceptor_);
      }
    });
  }

  virtual void close() override {
    close_now();
    if (accept_started_.load(std::memory_order_acquire)) {
      acceptor_close_future_.wait();
    }
  }
  virtual ~tcp_server_acceptor() = default;
  using server_acceptor_base::server_acceptor_base;

  std::optional<asio::ip::tcp::acceptor> acceptor_;
  coro_io::ExecutorWrapper<>* executor_ = nullptr;
  std::promise<void> acceptor_close_waiter_;
  std::future<void> acceptor_close_future_ =
      acceptor_close_waiter_.get_future();
  std::atomic<bool> accept_started_ = false;
};
}  // namespace coro_io
