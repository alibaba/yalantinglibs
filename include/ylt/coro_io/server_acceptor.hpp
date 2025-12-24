#pragma once
#include <async_simple/Executor.h>
#include <async_simple/coro/Lazy.h>

#include <asio/ip/tcp.hpp>
#include <charconv>
#include <cstdint>
#include <optional>
#include <string>
#include <system_error>

#include "asio/ip/address.hpp"
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
    if (port_ == 0) {
      if (size_t pos = address.rfind(':'); pos != std::string::npos) {
        auto port_sv = std::string_view(address).substr(pos + 1);

        uint16_t port;
        auto [ptr, ec] = std::from_chars(
            port_sv.data(), port_sv.data() + port_sv.size(), port, 10);
        if (ec != std::errc{}) {
          address_ = std::string{address};
          return;
        }

        port_ = port;
        address = address.substr(0, pos);
        if (address.front() == '[') {
          if (address.size() > 2)
            address = address.substr(1, address.size() - 2);
        }
      }
    }

    address_ = std::string{address};
  }
  uint16_t port_;
  std::string address_;
  coro_io::io_context_pool* pool_ = nullptr;
};

struct tcp_server_acceptor : public server_acceptor_base {
  virtual listen_errc listen() override {
    acceptor_ =
        asio::ip::tcp::acceptor{pool_->get_executor()->get_asio_executor()};
    ELOG_INFO << "begin to listen";
    using asio::ip::tcp;
    asio::error_code ec;
    asio::ip::tcp::resolver::query query(address_, std::to_string(port_));
    asio::ip::tcp::resolver resolver(acceptor_->get_executor());
    asio::ip::tcp::resolver::iterator it = resolver.resolve(query, ec);

    asio::ip::tcp::resolver::iterator it_end;
    if (ec || it == it_end) {
      ELOG_ERROR << "resolve address " << address_
                 << " error: " << ec.message();
      return listen_errc::bad_address;
    }

    auto endpoint = it->endpoint();
    acceptor_->open(endpoint.protocol(), ec);
    if (ec) {
      ELOG_ERROR << "open failed, error: " << ec.message();
      return listen_errc::open_error;
    }
#ifdef __GNUC__
    acceptor_->set_option(tcp::acceptor::reuse_address(true), ec);
#endif
    acceptor_->bind(endpoint, ec);
    if (ec) {
      ELOG_ERROR << "bind port " << port_ << " error: " << ec.message();
      acceptor_->cancel(ec);
      acceptor_->close(ec);
      return listen_errc::address_in_used;
    }
#ifdef _MSC_VER
    acceptor_.set_option(tcp::acceptor::reuse_address(true));
#endif
    acceptor_->listen(asio::socket_base::max_listen_connections, ec);
    if (ec) {
      ELOG_ERROR << "port " << port_ << " listen error: " << ec.message();
      acceptor_->cancel(ec);
      acceptor_->close(ec);
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
    assert(acceptor_ != std::nullopt);
    auto executor = pool_->get_executor();
    asio::ip::tcp::socket socket(executor->get_asio_executor());
    ELOG_INFO << "start accepting from acceptor: " << address_ << ":" << port_;
    auto error = co_await coro_io::async_accept(*acceptor_, socket);
    ELOG_INFO << "get connection from acceptor: " << address_ << ":" << port_;
    if (error) {
      ELOG_ERROR << "accept error: " << error.message();
      if (error == asio::error::operation_aborted ||
          error == asio::error::bad_descriptor) {
        acceptor_close_waiter_.set_value();
      }
      co_return ylt::expected<coro_io::socket_wrapper_t, std::error_code>{
          ylt::unexpected<std::error_code>{error}};
    }
    else {
      co_return coro_io::socket_wrapper_t{std::move(socket), executor};
    }
  }

  virtual void close() override {
    asio::dispatch(acceptor_->get_executor(), [this]() {
      asio::error_code ec;
      (void)acceptor_->cancel(ec);
      (void)acceptor_->close(ec);
    });
    acceptor_close_waiter_.get_future().wait();
  }
  virtual ~tcp_server_acceptor() = default;
  using server_acceptor_base::server_acceptor_base;

  std::optional<asio::ip::tcp::acceptor> acceptor_;
  std::promise<void> acceptor_close_waiter_;
};
}  // namespace coro_io
