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
#ifndef CORO_RPC_SERVERTESTER_HPP
#define CORO_RPC_SERVERTESTER_HPP
#include <async_simple/coro/Lazy.h>

#include <coro_rpc/coro_rpc_client.hpp>
#include <future>
#include <ostream>
#include <string>

#include "doctest.h"
#include "inject_action.hpp"
#include "logging/easylog.hpp"
#include "rpc_api.hpp"

#ifdef _MSC_VER
#define CORO_RPC_FUNCTION_SIGNATURE __FUNCSIG__
#else
#define CORO_RPC_FUNCTION_SIGNATURE __PRETTY_FUNCTION__
#endif

using namespace std::string_literals;
using namespace async_simple::coro;
using namespace coro_rpc;
using namespace std::chrono_literals;

struct TesterConfig {
  TesterConfig() = default;
  TesterConfig(TesterConfig &c) {
    async_start = c.sync_client;
    enable_heartbeat = c.enable_heartbeat;
    use_ssl = c.use_ssl;
    sync_client = c.sync_client;
    use_outer_io_context = c.use_outer_io_context;
    port = c.port;
    conn_timeout_duration = c.conn_timeout_duration;
  }
  bool async_start;
  bool enable_heartbeat;
  bool use_ssl;
  bool sync_client;
  bool use_outer_io_context;
  unsigned short port;
  std::chrono::steady_clock::duration conn_timeout_duration =
      std::chrono::seconds(0);

  friend std::ostream &operator<<(std::ostream &os,
                                  const TesterConfig &config) {
    os << std::boolalpha;
    os << "async_start: " << config.async_start << ";"
       << " enable_heartbeat: " << config.enable_heartbeat << ";"
       << " use_ssl: " << config.use_ssl << ";"
       << " sync_client: " << config.sync_client << ";"
       << " use_outer_io_context: " << config.use_outer_io_context << ";"
       << " port: " << config.port << ";";
    os << " conn_timeout_duration: ";
    auto val = std::chrono::duration_cast<std::chrono::milliseconds>(
                   config.conn_timeout_duration)
                   .count();
    os << val << "ms"
       << ";";
    return os;
  }
};

struct ServerTester : TesterConfig {
  ServerTester(TesterConfig config)
      : TesterConfig(config), port_(std::to_string(config.port)) {
    if (use_outer_io_context) {
      std::promise<void> promise;
      auto future = promise.get_future();
      thd_ = std::thread([this, &promise]() {
        asio::io_context::work work(io_context_);
        promise.set_value();
        io_context_.run();
      });
      future.wait();
    }

    std::stringstream ss;
    ss << config;
    conf_str_ = ss.str();
  }
  ~ServerTester() {
    if (use_outer_io_context) {
      io_context_.stop();
      thd_.join();
    }
    g_action = coro_rpc::inject_action::nothing;
  }
  void run() {
    easylog::info("run test: heartbeat={}, ssl={}", enable_heartbeat, use_ssl);
    register_all_function();
    test_all();
    remove_all_rpc_function();
  }
  virtual void test_all() {
    test_connect_timeout();
    test_function_not_registered();
    test_function_registered();
    if (sync_client) {
      // only sync_call implement inject behavior
      test_client_send_bad_header();
      test_client_send_bad_magic_num();
      test_client_send_header_length_is_0();
      test_client_close_socket_after_send_header();
      test_client_close_socket_after_send_partial_header();
      test_client_close_socket_after_send_payload();
    }
    test_heartbeat();
    test_call_function_with_long_response_time();
    test_call_with_large_buffer();
  }
  std::shared_ptr<coro_rpc_client> create_client(
      inject_action action = inject_action::nothing) {
    std::shared_ptr<coro_rpc_client> client;
    if (use_outer_io_context) {
      client = std::make_shared<coro_rpc_client>(io_context_, g_client_id++);
    }
    else {
      client = std::make_shared<coro_rpc_client>(g_client_id++);
    }
#ifdef ENABLE_SSL
    if (use_ssl) {
      bool ok = client->init_ssl("../openssl_files", "server.crt");
      REQUIRE_MESSAGE(ok == true, "init ssl fail, please check ssl config");
    }
#endif
    g_action = action;
    std::errc ec;
    if (sync_client) {
      ec = client->sync_connect("127.0.0.1", port_);
    }
    else {
      ec = syncAwait(client->connect("127.0.0.1", port_));
    }

    REQUIRE_MESSAGE(ec == err_ok, std::to_string(client->get_client_id())
                                      .append(" not connected ")
                                      .append(conf_str_));
    return client;
  }
  template <auto func, typename... Args>
  decltype(auto) call(std::shared_ptr<coro_rpc_client> client, Args &&...args) {
    easylog::info("{} client_id {} call {}",
                  sync_client ? "sync_client" : "async_client",
                  client->get_client_id(), coro_rpc::get_func_name<func>());
    if (sync_client) {
      return client->sync_call<func>(std::forward<Args>(args)...);
    }
    else {
      return syncAwait(
          client->template call<func>(std::forward<Args>(args)...));
    }
  }

  void test_function_not_registered() {
    g_action = {};
    remove_handler<async_hi>();
    easylog::info("run {}", __func__);
    auto client = create_client();
    auto ret = call<async_hi>(client);
    REQUIRE_MESSAGE(
        ret.error().code == std::errc::function_not_supported,
        std::to_string(client->get_client_id()).append(ret.error().msg));
    if (!use_ssl) {
      CHECK(client->has_closed() == false);
      ret = call<async_hi>(client);
      CHECK(client->has_closed() == true);
      ret = call<async_hi>(client);
      REQUIRE_MESSAGE(ret.error().code == std::errc::io_error, ret.error().msg);
      CHECK(client->has_closed() == true);
    }
    register_handler<async_hi>();
  }
  virtual void register_all_function() {
    g_action = {};
    easylog::info("run {}", __func__);
    register_handler<async_hi, large_arg_fun, client_hello>();
    register_handler<long_run_func>();
    register_handler<&ns_login::LoginService::login>(&login_service_);
    register_handler<&HelloService::hello>(&hello_service_);
    register_handler<hello>();
  }

  virtual void remove_all_rpc_function() {
    g_action = {};
    easylog::info("run {}", __func__);
    remove_handler<async_hi>();
    remove_handler<large_arg_fun>();
    remove_handler<client_hello>();
    remove_handler<long_run_func>();
    remove_handler<&ns_login::LoginService::login>();
    remove_handler<hello>();
    remove_handler<&HelloService::hello>();
  }
  void test_function_registered() {
    g_action = {};
    easylog::info("run {}", __func__);
    auto client = create_client();
    {
      auto ret = call<async_hi>(client);
      if (!ret) {
        easylog::warn(
            "{}", std::to_string(client->get_client_id()) + ret.error().msg);
      }
      CHECK(ret.value() == "async hi"s);
    }
    {
      auto ret = call<hello>(client);
      if (!ret) {
        easylog::warn(
            "{}", std::to_string(client->get_client_id()) + ret.error().msg);
      }
      CHECK(ret.value() == "hello"s);
    }
    {
      auto ret = call<&HelloService::hello>(client);
      if (!ret) {
        easylog::warn(
            "{}", std::to_string(client->get_client_id()) + ret.error().msg);
      }
      CHECK(ret.value() == "hello"s);
    }
    {
      auto ret = call<&ns_login::LoginService::login>(client, "foo"s, "bar"s);
      if (!ret) {
        easylog::warn(
            "{}", std::to_string(client->get_client_id()) + ret.error().msg);
      }
      CHECK(ret.value() == true);
    }
  }
  void test_client_send_bad_header() {
    g_action = {};
    easylog::info("run {}", __func__);
    auto client = create_client(inject_action::client_send_bad_header);
    auto ret = call<client_hello>(client);
    REQUIRE_MESSAGE(
        ret.error().code == std::errc::io_error,
        std::to_string(client->get_client_id()).append(ret.error().msg));
  }
  void test_client_send_bad_magic_num() {
    g_action = {};
    easylog::info("run {}", __func__);
    auto client = create_client(inject_action::client_send_bad_magic_num);
    auto ret = call<client_hello>(client);
    REQUIRE_MESSAGE(
        ret.error().code == std::errc::io_error,
        std::to_string(client->get_client_id()).append(ret.error().msg));
  }
  void test_client_send_header_length_is_0() {
    g_action = {};
    easylog::info("run {}", __func__);
    auto client = create_client(inject_action::client_send_header_length_0);
    auto ret = call<client_hello>(client);
    REQUIRE_MESSAGE(
        ret.error().code == std::errc::io_error,
        std::to_string(client->get_client_id()).append(ret.error().msg));
  }
  void test_client_close_socket_after_send_header() {
    g_action = {};
    easylog::info("run {}", __func__);
    auto client =
        create_client(inject_action::client_close_socket_after_send_header);
    auto ret = call<client_hello>(client);
    REQUIRE_MESSAGE(
        ret.error().code == std::errc::io_error,
        std::to_string(client->get_client_id()).append(ret.error().msg));
  }
  void test_client_close_socket_after_send_partial_header() {
    g_action = {};
    easylog::info("run {}", __func__);
    auto client = create_client(
        inject_action::client_close_socket_after_send_partial_header);
    auto ret = call<client_hello>(client);
    REQUIRE_MESSAGE(
        ret.error().code == std::errc::io_error,
        std::to_string(client->get_client_id()).append(ret.error().msg));
  }
  void test_client_close_socket_after_send_payload() {
    g_action = {};
    easylog::info("run {}", __func__);
    auto client =
        create_client(inject_action::client_close_socket_after_send_payload);
    auto ret = call<client_hello>(client);
    REQUIRE_MESSAGE(
        ret.error().code == std::errc::io_error,
        std::to_string(client->get_client_id()).append(ret.error().msg));
  }

  void test_heartbeat() {
    easylog::info("run {}", __func__);
    auto client = create_client(inject_action::nothing);

    auto ret = call<async_hi>(client);
    CHECK(ret.value() == "async hi"s);

    std::this_thread::sleep_for(60ms);

    ret = call<async_hi>(client);
    if (enable_heartbeat) {
      REQUIRE_MESSAGE(
          ret.error().code == std::errc::io_error,
          std::to_string(client->get_client_id()).append(ret.error().msg));
    }
    else {
      CHECK(ret.value() == "async hi"s);
    }
    easylog::info("test heartbeat done");
  }
  void test_call_function_with_long_response_time() {
    easylog::info("run {}", __func__);
    auto client = create_client(inject_action::nothing);
    auto ret = call<long_run_func>(client, 1);
    CHECK(ret.value() == 1);
  }
  void test_call_with_large_buffer() {
    easylog::info("run {}", __func__);
    auto client = create_client(inject_action::nothing);
    std::string arg;
    arg.resize(2048);
    auto ret = call<large_arg_fun>(client, arg);
    CHECK(ret.value() == arg);
  }

  void test_connect_timeout() {
    g_action = {};
    easylog::info("run {}", __func__);
    if (sync_client) {
      return;
    }
    auto init_client = [this]() {
      std::shared_ptr<coro_rpc_client> client;
      if (use_outer_io_context) {
        client = std::make_shared<coro_rpc_client>(io_context_, g_client_id++);
      }
      else {
        client = std::make_shared<coro_rpc_client>(g_client_id++);
      }
#ifdef ENABLE_SSL
      if (use_ssl) {
        bool ok = client->init_ssl("../openssl_files", "server.crt");
        REQUIRE_MESSAGE(ok == true, "init ssl fail, please check ssl config");
      }
#endif
      return client;
    };
    auto client = init_client();
    std::errc ec;
    // ec = syncAwait(client->connect("127.0.0.1", port, 0ms));
    // CHECK_MESSAGE(ec == std::errc::timed_out, make_error_code(ec).message());
    auto client2 = init_client();
    ec = syncAwait(client2->connect("10.255.255.1", port_, 5ms));
    CHECK_MESSAGE(ec == std::errc::timed_out,
                  std::to_string(client->get_client_id())
                      .append(make_error_code(ec).message()));
  }

  template <auto func, typename... Args>
  void test_call_with_delay_func(Args... args) {
    g_action = {};
    easylog::info("run {}", __func__);
    auto client = create_client();
    auto ret = call<func>(client, std::forward<Args>(args)...);
    CHECK(ret.has_value());
  }

  template <auto func, typename... Args>
  void test_call_with_delay_func_client_read_length_error(Args... args) {
    g_action = {};
    easylog::info("run {}", CORO_RPC_FUNCTION_SIGNATURE);
    auto client = this->create_client();
    g_action = inject_action::close_socket_after_read_header;
    auto ret = this->template call<func>(client, std::forward<Args>(args)...);
    REQUIRE_MESSAGE(
        ret.error().code == std::errc::io_error,
        std::to_string(client->get_client_id()).append(ret.error().msg));
  };

  template <auto func, typename... Args>
  void test_call_with_delay_func_client_read_body_error(Args... args) {
    g_action = {};
    easylog::info("run {}", CORO_RPC_FUNCTION_SIGNATURE);
    auto client = this->create_client();
    g_action = inject_action::close_socket_after_send_length;
    auto ret = this->template call<func>(client, std::forward<Args>(args)...);
    REQUIRE_MESSAGE(
        ret.error().code == std::errc::io_error,
        std::to_string(client->get_client_id()).append(ret.error().msg));
  };

  template <auto func, typename... Args>
  void test_call_with_delay_func_server_timeout_due_to_heartbeat(Args... args) {
    g_action = {};
    easylog::info("run {}", CORO_RPC_FUNCTION_SIGNATURE);
    auto client = this->create_client();
    auto ret = this->template call<func>(client, std::forward<Args>(args)...);
    REQUIRE(!ret);
    REQUIRE_MESSAGE(
        ret.error().code == std::errc::io_error,
        std::to_string(client->get_client_id()).append(ret.error().msg));
  };
  asio::io_context io_context_;
  std::string port_;
  std::thread thd_;
  ns_login::LoginService login_service_;
  HelloService hello_service_;
  std::string conf_str_;
};
#endif  // CORO_RPC_SERVERTESTER_HPP
