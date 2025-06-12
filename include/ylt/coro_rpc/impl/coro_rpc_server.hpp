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
#include <async_simple/coro/Lazy.h>

#include <asio/dispatch.hpp>
#include <asio/error_code.hpp>
#include <asio/io_context.hpp>
#include <atomic>
#include <charconv>
#include <condition_variable>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <future>
#include <memory>
#include <mutex>
#include <system_error>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <vector>
#include <ylt/easylog.hpp>

#include "async_simple/Common.h"
#include "async_simple/Promise.h"
#include "async_simple/util/move_only_function.h"
#include "common_service.hpp"
#include "coro_connection.hpp"
#include "ylt/coro_io/coro_io.hpp"
#include "ylt/coro_io/io_context_pool.hpp"
#include "ylt/coro_rpc/impl/expected.hpp"
namespace coro_rpc {
/*!
 * ```cpp
 * #include <ylt/coro_rpc/coro_rpc_server.hpp>
 * inline std::string hello_coro_rpc() { return "hello coro_rpc"; }
 * int main(){
 *   register_handler<hello_coro_rpc>();
 *   coro_rpc_server server(std::thread::hardware_concurrency(), 9000);
 *   server.start();
 *   return 0;
 * }
 * ```
 */

template <typename server_config>
class coro_rpc_server_base {
  //!< Server state
  enum class stat {
    init = 0,  // server hasn't started/stopped.
    started,   // server is started
    stop       // server is stopping/stopped
  };

 public:
  /*!
   * TODO: add doc
   * @param thread_num the number of io_context.
   * @param port the server port to listen.
   * @param listen address of server
   * @param conn_timeout_duration client connection timeout. 0 for no timeout.
   *                              default no timeout.
   * @param is_enable_tcp_no_delay is tcp socket allow
   */
  coro_rpc_server_base(size_t thread_num = std::thread::hardware_concurrency(),
                       unsigned short port = 9001,
                       std::string address = "0.0.0.0",
                       std::chrono::steady_clock::duration
                           conn_timeout_duration = std::chrono::seconds(0),
                       bool is_enable_tcp_no_delay = true)
      : pool_(thread_num),
        acceptor_(pool_.get_executor()->get_asio_executor()),
        port_(port),
        conn_timeout_duration_(conn_timeout_duration),
        flag_{stat::init},
        is_enable_tcp_no_delay_(is_enable_tcp_no_delay) {
    init_address(std::move(address));
  }

  coro_rpc_server_base(size_t thread_num, std::string address,
                       std::chrono::steady_clock::duration
                           conn_timeout_duration = std::chrono::seconds(0),
                       bool is_enable_tcp_no_delay = true)
      : pool_(thread_num),
        acceptor_(pool_.get_executor()->get_asio_executor()),
        conn_timeout_duration_(conn_timeout_duration),
        flag_{stat::init},
        is_enable_tcp_no_delay_(is_enable_tcp_no_delay) {
    init_address(std::move(address));
  }

  coro_rpc_server_base(const server_config &config)
      : pool_(config.thread_num),
        acceptor_(pool_.get_executor()->get_asio_executor()),
        port_(config.port),
        conn_timeout_duration_(config.conn_timeout_duration),
        flag_{stat::init},
        is_enable_tcp_no_delay_(config.is_enable_tcp_no_delay) {
#ifdef YLT_ENABLE_SSL
    if (config.ssl_config) {
      init_ssl_context_helper(context_, config.ssl_config.value());
    }
    else if (config.ibv_config) {
      init_ibv(config.ibv_config);
    }
#endif
    init_address(config.address);
  }

  ~coro_rpc_server_base() {
    ELOG_INFO << "coro_rpc_server will quit";
    stop();
  }

#ifdef YLT_ENABLE_SSL
  void init_ssl(const ssl_configure &conf) {
    use_ssl_ = init_ssl_context_helper(context_, conf);
  }
#endif
#ifdef YLT_ENABLE_IBV
  void init_ibv(const coro_io::ibverbs_config &conf = {}) {
    ibv_config_ = conf;
  }
#endif

  /*!
   * Start the server in blocking mode
   *
   * If the port is used, it will return a non-empty error code.
   *
   * @return error code if start failed, otherwise block until server stop.
   */
  [[nodiscard]] coro_rpc::err_code start() noexcept {
    return async_start().get();
  }

 private:
  async_simple::Future<coro_rpc::err_code> make_error_future(
      coro_rpc::err_code &&err) {
    async_simple::Promise<coro_rpc::err_code> p;
    p.setValue(std::move(err));
    return p.getFuture();
  }

 public:
  async_simple::Future<coro_rpc::err_code> async_start() noexcept {
    {
      std::unique_lock lock(start_mtx_);
      if (flag_ != stat::init) {
        if (flag_ == stat::started) {
          ELOG_INFO << "start again";
        }
        else if (flag_ == stat::stop) {
          ELOG_INFO << "has stoped";
        }
        return make_error_future(
            coro_rpc::err_code{coro_rpc::errc::server_has_ran});
      }
      errc_ = listen();
      if (!errc_) {
        if constexpr (requires(typename server_config::executor_pool_t &pool) {
                        pool.run();
                      }) {
          thd_ = std::thread([this] {
            pool_.run();
          });
        }
        flag_ = stat::started;
      }
      else {
        flag_ = stat::stop;
      }
    }
    if (!errc_) {
      async_simple::Promise<coro_rpc::err_code> promise;
      auto future = promise.getFuture();
      accept().start([this, p = std::move(promise)](auto &&res) mutable {
        ELOG_ERROR << "server quit!";
        if (res.hasError()) {
          stop();
          errc_ = coro_rpc::err_code{coro_rpc::errc::io_error};
          p.setValue(errc_);
        }
        else {
          p.setValue(res.value());
        }
      });
      return std::move(future);
    }
    else {
      return make_error_future(coro_rpc::err_code{errc_});
    }
  }

  /*!
   * Stop server
   *
   * Block and wait server stops.
   */
  void stop() {
    std::unique_lock lock(start_mtx_);
    if (flag_ == stat::stop) {
      return;
    }

    ELOG_INFO << "begin to stop coro_rpc_server";

    if (flag_ == stat::started) {
      close_acceptor();
      {
        std::unique_lock lock(conns_mtx_);
        ELOG_INFO << "total connection count: " << conns_.size();
        for (auto &conn : conns_) {
          if (!conn.second->has_closed()) {
            conn.second->async_close();
          }
        }

        conns_.clear();
      }

      ELOG_INFO << "wait for server's thread-pool finish all work.";
      pool_.stop();
      ELOG_INFO << "server's thread-pool finished.";
    }
    if (thd_.joinable()) {
      thd_.join();
    }

    ELOG_INFO << "stop coro_rpc_server ok.";
    flag_ = stat::stop;
  }

  /*!
   * Get listening port
   * @return
   */
  uint16_t port() const { return port_; };
  std::string_view address() const { return address_; }
  coro_rpc::err_code get_errc() const { return errc_; }

  template <typename... ServerType>
  void add_subserver(
      std::function<void(coro_io::socket_wrapper_t &&socket,
                         std::string_view magic_number, ServerType &...server)>
          dispatcher,
      std::unique_ptr<ServerType>... server) {
    connection_transfer_ = [dispatcher = std::move(dispatcher),
                            server = std::make_tuple(std::move(server)...)](
                               coro_io::socket_wrapper_t &&socket,
                               std::string_view magic_number,
                               int index = -1) mutable {
      std::apply(
          [&dispatcher, &socket, magic_number](auto &...server) {
            dispatcher(std::move(socket), magic_number, *server...);
          },
          server);
    };
  }

  /*!
   * Register RPC service functions (member function)
   *
   * Before RPC server started, all RPC service functions must be registered.
   * All you need to do is fill in the template parameters with the address of
   * your own RPC functions. If RPC function is registered twice, the program
   * will be terminate with exit code `EXIT_FAILURE`.
   *
   * Note: All functions must be member functions of the same class.
   *
   * ```cpp
   * class test_class {
   *  public:
   *  void plus_one(int val) {}
   *  std::string get_str(std::string str) { return str; }
   * };
   * int main() {
   *   test_class obj{};
   *   // register member functions
   *   register_handler<&test_class::plus_one, &test_class::get_str>(&obj);
   *   return 0;
   * }
   * ```
   *
   * @tparam first the address of RPC function. e.g. `&foo::bar`
   * @tparam func the address of RPC function. e.g. `&foo::bar`
   * @param self the object pointer corresponding to these member functions
   */

  template <auto first, auto... functions>
  void register_handler(util::class_type_t<decltype(first)> *self) {
    router_.template register_handler<first, functions...>(self);
  }

  template <auto first>
  void register_handler(util::class_type_t<decltype(first)> *self,
                        const auto &key) {
    router_.template register_handler<first>(self, key);
  }

  /*!
   * Register RPC service functions (non-member function)
   *
   * Before RPC server started, all RPC service functions must be registered.
   * All you need to do is fill in the template parameters with the address of
   * your own RPC functions. If RPC function is registered twice, the program
   * will be terminate with exit code `EXIT_FAILURE`.
   *
   * ```cpp
   * // RPC functions (non-member function)
   * void hello() {}
   * std::string get_str() { return ""; }
   * int add(int a, int b) {return a + b; }
   * int main() {
   *   register_handler<hello>();         // register one RPC function at once
   *   register_handler<get_str, add>();  // register multiple RPC functions at
   * once return 0;
   * }
   * ```
   *
   * @tparam first the address of RPC function. e.g. `foo`, `bar`
   * @tparam func the address of RPC function. e.g. `foo`, `bar`
   */

  template <auto... functions>
  void register_handler() {
    router_.template register_handler<functions...>();
  }

  template <auto func>
  void register_handler(const auto &key) {
    router_.template register_handler<func>(key);
  }

  auto &get_io_context_pool() noexcept { return pool_; }

 private:
  coro_rpc::err_code listen() {
    ELOG_INFO << "begin to listen";
    using asio::ip::tcp;
    asio::error_code ec;
    asio::ip::tcp::resolver::query query(address_, std::to_string(port_));
    asio::ip::tcp::resolver resolver(acceptor_.get_executor());
    asio::ip::tcp::resolver::iterator it = resolver.resolve(query, ec);

    asio::ip::tcp::resolver::iterator it_end;
    if (ec || it == it_end) {
      ELOG_ERROR << "resolve address " << address_
                 << " error: " << ec.message();
      return coro_rpc::errc::bad_address;
    }

    auto endpoint = it->endpoint();
    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
      ELOG_ERROR << "open failed, error: " << ec.message();
      return coro_rpc::errc::open_error;
    }
#ifdef __GNUC__
    acceptor_.set_option(tcp::acceptor::reuse_address(true), ec);
#endif
    acceptor_.bind(endpoint, ec);
    if (ec) {
      ELOG_ERROR << "bind port " << port_.load() << " error: " << ec.message();
      acceptor_.cancel(ec);
      acceptor_.close(ec);
      return coro_rpc::errc::address_in_used;
    }
#ifdef _MSC_VER
    acceptor_.set_option(tcp::acceptor::reuse_address(true));
#endif
    acceptor_.listen(asio::socket_base::max_listen_connections, ec);
    if (ec) {
      ELOG_ERROR << "port " << port_.load()
                 << " listen error: " << ec.message();
      acceptor_.cancel(ec);
      acceptor_.close(ec);
      return coro_rpc::errc::listen_error;
    }

    auto end_point = acceptor_.local_endpoint(ec);
    if (ec) {
      ELOG_ERROR << "get local endpoint port " << port_.load()
                 << " error: " << ec.message();
      return coro_rpc::errc::address_in_used;
    }
    port_ = end_point.port();

    ELOG_INFO << "listen port " << port_.load() << " successfully";
    return {};
  }

  async_simple::coro::Lazy<coro_rpc::err_code> accept() {
    for (;;) {
      auto executor = pool_.get_executor();
      asio::ip::tcp::socket socket(executor->get_asio_executor());
      auto error = co_await coro_io::async_accept(acceptor_, socket);
#ifdef UNIT_TEST_INJECT
      if (g_action == inject_action::force_inject_server_accept_error) {
        asio::error_code ignored_ec;
        socket.shutdown(asio::ip::tcp::socket::shutdown_both, ignored_ec);
        socket.close(ignored_ec);
        error = make_error_code(std::errc::io_error);
        // only inject once
        g_action = inject_action::nothing;
      }
#endif
      if (error) {
        if (error == asio::error::operation_aborted) {
          ELOG_INFO << "server was canceled:" << error.message();
        }
        else {
          ELOG_ERROR << "server accept failed:" << error.message();
        }
        if (error == asio::error::operation_aborted ||
            error == asio::error::bad_descriptor) {
          acceptor_close_waiter_.set_value();
          co_return coro_rpc::errc::operation_canceled;
        }
        continue;
      }

      int64_t conn_id = ++conn_id_;
      ELOG_INFO << "new client conn_id " << conn_id << " coming";
      if (is_enable_tcp_no_delay_) {
        socket.set_option(asio::ip::tcp::no_delay(true), error);
      }
      coro_io::socket_wrapper_t wrapper;
      bool init_failed = false;
      do {
#ifdef YLT_ENABLE_SSL
        if (use_ssl_) {
          wrapper = {std::move(socket), executor, context_};
          break;
        }
#endif
#ifdef YLT_ENABLE_IBV
        if (ibv_config_.has_value()) {
          try {
            wrapper = {std::move(socket), executor, *ibv_config_, nullptr,
                       nullptr};
          } catch (...) {
            init_failed = true;
          }
          break;
        }
#endif
        wrapper = {std::move(socket), executor};
      } while (false);
      if (init_failed)
        continue;
      auto conn = std::make_shared<coro_connection>(std::move(wrapper),
                                                    conn_timeout_duration_);
      conn->set_quit_callback(
          [this](const uint64_t &id) {
            std::unique_lock lock(conns_mtx_);
            conns_.erase(id);
          },
          conn_id);
      if (connection_transfer_) {
        conn->set_transfer_callback(&connection_transfer_);
      }
      {
        std::unique_lock lock(conns_mtx_);
        conns_.emplace(conn_id, conn);
      }
      start_one(conn).via(conn->get_executor()).detach();
    }
  }

  async_simple::coro::Lazy<void> start_one(auto conn) noexcept {
    co_await conn->template start<typename server_config::rpc_protocol>(
        router_);
  }

  void close_acceptor() {
    asio::dispatch(acceptor_.get_executor(), [this]() {
      asio::error_code ec;
      (void)acceptor_.cancel(ec);
      (void)acceptor_.close(ec);
    });
    acceptor_close_waiter_.get_future().wait();
  }

  void init_address(std::string address) {
    if (size_t pos = address.find(':'); pos != std::string::npos) {
      auto port_sv = std::string_view(address).substr(pos + 1);

      uint16_t port;
      auto [ptr, ec] = std::from_chars(
          port_sv.data(), port_sv.data() + port_sv.size(), port, 10);
      if (ec != std::errc{}) {
        address_ = std::move(address);
        return;
      }

      port_ = port;
      address = address.substr(0, pos);
    }

    address_ = std::move(address);
  }

  typename server_config::executor_pool_t pool_;
  asio::ip::tcp::acceptor acceptor_;
  std::promise<void> acceptor_close_waiter_;

  std::thread thd_;
  stat flag_;

  std::mutex start_mtx_;
  uint64_t conn_id_ = 0;
  std::unordered_map<uint64_t, std::shared_ptr<coro_connection>> conns_;
  std::mutex conns_mtx_;

  typename server_config::rpc_protocol::router router_;

  std::atomic<uint16_t> port_;
  std::string address_;
  bool is_enable_tcp_no_delay_;
  coro_rpc::err_code errc_ = {};
  std::chrono::steady_clock::duration conn_timeout_duration_;

  async_simple::util::move_only_function<void(coro_io::socket_wrapper_t &&soc,
                                              std::string_view magic_number)>
      connection_transfer_;

#ifdef YLT_ENABLE_SSL
  asio::ssl::context context_{asio::ssl::context::sslv23};
  bool use_ssl_ = false;
#endif
#ifdef YLT_ENABLE_IBV
  std::optional<coro_io::ibverbs_config> ibv_config_;
#endif
};
}  // namespace coro_rpc
