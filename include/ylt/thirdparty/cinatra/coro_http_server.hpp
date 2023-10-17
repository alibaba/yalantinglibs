#pragma once

#include <asio/dispatch.hpp>
#include <cstdint>
#include <mutex>
#include <type_traits>

#include "asio/streambuf.hpp"
#include "async_simple/Promise.h"
#include "async_simple/coro/Lazy.h"
#include "cinatra/coro_http_response.hpp"
#include "cinatra/coro_http_router.hpp"
#include "cinatra_log_wrapper.hpp"
#include "coro_http_connection.hpp"
#include "ylt/coro_io/coro_io.hpp"
#include "ylt/coro_io/io_context_pool.hpp"

namespace cinatra {
class coro_http_server {
 public:
  coro_http_server(size_t thread_num, unsigned short port)
      : pool_(thread_num),
        port_(port),
        acceptor_(pool_.get_executor()->get_asio_executor()) {}

  ~coro_http_server() {
    CINATRA_LOG_INFO << "coro_http_server will quit";
    stop();
  }

  void set_no_delay(bool r) { no_delay_ = r; }

#ifdef CINATRA_ENABLE_SSL
  void init_ssl(const std::string &cert_file, const std::string &key_file,
                const std::string &passwd) {
    cert_file_ = cert_file;
    key_file_ = key_file;
    passwd_ = passwd;
    use_ssl_ = true;
  }
#endif

  // only call once, not thread safe.
  std::errc sync_start() noexcept {
    auto ret = async_start();
    ret.wait();
    return ret.value();
  }

  // only call once, not thread safe.
  async_simple::Future<std::errc> async_start() {
    auto ec = listen();

    async_simple::Promise<std::errc> promise;
    auto future = promise.getFuture();

    if (ec == std::errc{}) {
      thd_ = std::thread([this] {
        pool_.run();
      });

      accept().start([p = std::move(promise)](auto &&res) mutable {
        if (res.hasError()) {
          p.setValue(std::errc::io_error);
        }
        else {
          p.setValue(res.value());
        }
      });
    }
    else {
      promise.setValue(ec);
    }

    return std::move(future);
  }

  // only call once, not thread safe.
  void stop() {
    if (!thd_.joinable()) {
      return;
    }
    close_acceptor();

    // close current connections.
    {
      std::scoped_lock lock(conn_mtx_);
      for (auto &conn : connections_) {
        conn.second->close();
      }
      connections_.clear();
    }

    CINATRA_LOG_INFO << "wait for server's thread-pool finish all work.";
    pool_.stop();
    CINATRA_LOG_INFO << "server's thread-pool finished.";
    thd_.join();
    CINATRA_LOG_INFO << "stop coro_http_server ok";
  }

  // call it after server async_start or sync_start.
  uint16_t port() const { return port_; }

  template <http_method... method, typename Func>
  void set_http_handler(std::string key, Func handler) {
    static_assert(sizeof...(method) >= 1, "must set http_method");
    if constexpr (sizeof...(method) == 1) {
      (coro_http_router::instance().set_http_handler<method>(
           std::move(key), std::move(handler)),
       ...);
    }
    else {
      (coro_http_router::instance().set_http_handler<method>(key, handler),
       ...);
    }
  }

 private:
  std::errc listen() {
    CINATRA_LOG_INFO << "begin to listen";
    using asio::ip::tcp;
    auto endpoint = tcp::endpoint(tcp::v4(), port_);
    acceptor_.open(endpoint.protocol());
#ifdef __GNUC__
    acceptor_.set_option(tcp::acceptor::reuse_address(true));
#endif
    asio::error_code ec;
    acceptor_.bind(endpoint, ec);
    if (ec) {
      CINATRA_LOG_ERROR << "bind port: " << port_ << " error: " << ec.message();
      acceptor_.cancel(ec);
      acceptor_.close(ec);
      return std::errc::address_in_use;
    }
#ifdef _MSC_VER
    acceptor_.set_option(tcp::acceptor::reuse_address(true));
#endif
    acceptor_.listen();

    auto end_point = acceptor_.local_endpoint(ec);
    if (ec) {
      CINATRA_LOG_ERROR << "get local endpoint port: " << port_
                        << " error: " << ec.message();
      return std::errc::address_in_use;
    }
    port_ = end_point.port();

    CINATRA_LOG_INFO << "listen port " << port_ << " successfully";
    return {};
  }

  async_simple::coro::Lazy<std::errc> accept() {
    for (;;) {
      auto executor = pool_.get_executor();
      asio::ip::tcp::socket socket(executor->get_asio_executor());
      auto error = co_await coro_io::async_accept(acceptor_, socket);
      if (error) {
        CINATRA_LOG_INFO << "accept failed, error: " << error.message();
        if (error == asio::error::operation_aborted ||
            error == asio::error::bad_descriptor) {
          acceptor_close_waiter_.set_value();
          co_return std::errc::operation_canceled;
        }
        continue;
      }

      uint64_t conn_id = ++conn_id_;
      CINATRA_LOG_DEBUG << "new connection comming, id: " << conn_id;
      auto conn =
          std::make_shared<coro_http_connection>(executor, std::move(socket));
      if (no_delay_) {
        conn->tcp_socket().set_option(asio::ip::tcp::no_delay(true));
      }

#ifdef CINATRA_ENABLE_SSL
      if (use_ssl_) {
        conn->init_ssl(cert_file_, key_file_, passwd_);
      }
#endif

      conn->set_quit_callback(
          [this](const uint64_t &id) {
            std::scoped_lock lock(conn_mtx_);
            if (!connections_.empty())
              connections_.erase(id);
          },
          conn_id);

      {
        std::scoped_lock lock(conn_mtx_);
        connections_.emplace(conn_id, conn);
      }

      start_one(conn).via(&conn->get_executor()).detach();
    }
  }

  async_simple::coro::Lazy<void> start_one(
      std::shared_ptr<coro_http_connection> conn) noexcept {
    co_await conn->start();
  }

  void close_acceptor() {
    asio::dispatch(acceptor_.get_executor(), [this]() {
      asio::error_code ec;
      acceptor_.cancel(ec);
      acceptor_.close(ec);
    });
    acceptor_close_waiter_.get_future().wait();
  }

 private:
  coro_io::io_context_pool pool_;
  uint16_t port_;
  asio::ip::tcp::acceptor acceptor_;
  std::thread thd_;
  std::promise<void> acceptor_close_waiter_;
  bool no_delay_ = true;

  uint64_t conn_id_ = 0;
  std::unordered_map<uint64_t, std::shared_ptr<coro_http_connection>>
      connections_;
  std::mutex conn_mtx_;
#ifdef CINATRA_ENABLE_SSL
  std::string cert_file_;
  std::string key_file_;
  std::string passwd_;
  bool use_ssl_ = false;
#endif
};
}  // namespace cinatra