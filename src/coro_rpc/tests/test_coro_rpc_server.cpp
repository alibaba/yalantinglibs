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
#include <async_simple/coro/Collect.h>
#include <async_simple/coro/SyncAwait.h>

#include <coro_rpc/coro_rpc_client.hpp>
#include <coro_rpc/coro_rpc_server.hpp>
#include <thread>
#include <variant>

#include "ServerTester.hpp"
#include "async_simple/coro/Lazy.h"
#include "coro_rpc/coro_rpc/rpc_protocol.h"
#include "doctest.h"
#include "rpc_api.hpp"
#include "struct_pack/struct_pack.hpp"

async_simple::coro::Lazy<int> get_coro_value(int val) { co_return val; }

struct CoroServerTester : ServerTester {
  CoroServerTester(TesterConfig config)
      : ServerTester(config),
        server(2, config.port, config.conn_timeout_duration) {
#ifdef ENABLE_SSL
    if (use_ssl) {
      server.init_ssl_context(
          ssl_configure{"../openssl_files", "server.crt", "server.key"});
    }
#endif
    if (async_start) {
      // https://timsong-cpp.github.io/cppwp/n4861/temp.names#5.example-1
      // https://developercommunity.visualstudio.com/t/c2059-syntax-error-template-for-valid-template-mem/1632142
      /*
        template <class T> struct A {
          void f(int);
          template <class U> void f(U);
        };

        template <class T> void f(T t) {
          A<T> a;
          a.template f<>(t);                    // OK: calls template
          a.template f(t);                      // error: not a template-id
        }
      */
      server.async_start().template start<>([](auto &&) {
      });
    }
    else {
      thd = std::thread([&] {
        auto ec = server.start();
        REQUIRE(ec == std::errc{});
      });
    }

    CHECK_MESSAGE(server.wait_for_start(3s), "server start timeout");
  }
  ~CoroServerTester() {
    if (async_start) {
    }
    else {
      server.stop();
      thd.join();
    }
  }

  async_simple::coro::Lazy<int> get_value(int val) { co_return val; }

  void test_all() override {
    g_action = {};
    ELOGV(INFO, "run %s", __func__);
    test_coro_handler();
    ServerTester::test_all();
    test_function_not_registered();
    test_start_new_server_with_same_port();
    test_server_send_bad_rpc_result();
    test_server_send_no_body();
    this->test_call_with_delay_func<coro_fun_with_delay_return_void>();
    this->test_call_with_delay_func<coro_fun_with_delay_return_void_twice>();
    this->test_call_with_delay_func_client_read_length_error<
        coro_fun_with_delay_return_void>();
    this->test_call_with_delay_func_client_read_body_error<
        coro_fun_with_delay_return_void>();
    if (enable_heartbeat) {
      this->test_call_with_delay_func_server_timeout_due_to_heartbeat<
          coro_fun_with_delay_return_void_cost_long_time>();
    }
    this->test_call_with_delay_func<coro_fun_with_delay_return_string>();
    this->test_call_with_delay_func<coro_fun_with_delay_return_string_twice>();
  }
  void register_all_function() override {
    g_action = {};
    ELOGV(INFO, "run %s", __func__);
    server.regist_handler<async_hi, large_arg_fun, client_hello>();
    server.regist_handler<long_run_func>();
    server.regist_handler<&ns_login::LoginService::login>(&login_service_);
    server.regist_handler<&HelloService::hello>(&hello_service_);
    server.regist_handler<hello>();
    server.regist_handler<coro_fun_with_delay_return_void>();
    server.regist_handler<coro_fun_with_delay_return_void_twice>();
    server.regist_handler<coro_fun_with_delay_return_void_cost_long_time>();
    server.regist_handler<coro_fun_with_delay_return_string>();
    server.regist_handler<coro_fun_with_delay_return_string_twice>();
    server.regist_handler<coro_func>();
    server.regist_handler<&HelloService::coro_func>(&hello_service_);
    server.regist_handler<get_coro_value>();
    server.regist_handler<&CoroServerTester::get_value>(this);
  }

  void test_function_not_registered() {
    g_action = {};
    auto client = create_client();
    ELOGV(INFO, "run %s, client_id %d", __func__, client->get_client_id());
    auto ret = call<function_not_registered>(client);
    REQUIRE_MESSAGE(
        ret.error().code == std::errc::function_not_supported,
        std::to_string(client->get_client_id()).append(ret.error().msg));
    REQUIRE(client->has_closed() == true);
    ret = call<function_not_registered>(client);
    CHECK(client->has_closed() == true);
    ret = call<function_not_registered>(client);
    REQUIRE_MESSAGE(ret.error().code == std::errc::io_error, ret.error().msg);
    CHECK(client->has_closed() == true);
    server.regist_handler<function_not_registered>();
  }

  void test_server_start_again() {
    ELOGV(INFO, "run %s", __func__);
    std::errc ec;
    if (async_start) {
      ec = syncAwait(server.async_start());
    }
    else {
      ec = server.start();
    }
    REQUIRE_MESSAGE(ec == std::errc::io_error, make_error_code(ec).message());
  }

  void test_start_new_server_with_same_port() {
    ELOGV(INFO, "run %s", __func__);
    auto new_server = coro_rpc_server(2, std::stoi(this->port_));
    std::errc ec;
    if (async_start) {
      ec = syncAwait(new_server.async_start());
    }
    else {
      ec = new_server.start();
    }
    REQUIRE_MESSAGE(ec == std::errc::address_in_use,
                    make_error_code(ec).message());
  }
  void test_server_send_bad_rpc_result() {
    auto client = create_client(inject_action::server_send_bad_rpc_result);
    ELOGV(INFO, "run %s, client_id %d", __func__, client->get_client_id());
    auto ret = this->call<hi>(client);
    CHECK_MESSAGE(
        ret.error().code == std::errc::invalid_argument,
        std::to_string(client->get_client_id()).append(ret.error().msg));
    g_action = {};
  }

  void test_server_send_no_body() {
    auto client = create_client(inject_action::close_socket_after_send_length);
    ELOGV(INFO, "run %s, client_id %d", __func__, client->get_client_id());
    auto ret = this->template call<hello>(client);
    REQUIRE_MESSAGE(
        ret.error().code == std::errc::io_error,
        std::to_string(client->get_client_id()).append(ret.error().msg));
    g_action = {};
  }

  void test_coro_handler() {
    auto client = create_client(inject_action::nothing);
    ELOGV(INFO, "run %s, client_id %d", __func__, client->get_client_id());
    auto ret = this->template call<get_coro_value>(client, 42);
    CHECK(ret.value() == 42);

    auto ret1 = this->template call<&HelloService::coro_func>(client, 42);
    CHECK(ret1.value() == 42);

    auto ret2 = this->template call<&CoroServerTester::get_value>(client, 42);
    CHECK(ret2.value() == 42);

    auto ret3 = this->template call<coro_func>(client, 42);
    CHECK(ret3.value() == 42);
  }
  coro_rpc_server<> server;
  std::thread thd;
  HelloService hello_service_;
};
TEST_CASE("testing coro rpc server") {
  ELOGV(INFO, "run testing coro rpc server");
  unsigned short server_port = 8810;
  auto conn_timeout_duration = 300ms;
  std::vector<bool> switch_list{true, false};
  for (auto async_start : switch_list) {
    for (auto enable_heartbeat : switch_list) {
      for (auto use_ssl : switch_list) {
        TesterConfig config;
        config.async_start = async_start;
        config.enable_heartbeat = enable_heartbeat;
        config.use_ssl = use_ssl;
        config.sync_client = false;
        config.use_outer_io_context = false;
        config.port = server_port;
        if (enable_heartbeat) {
          config.conn_timeout_duration = conn_timeout_duration;
        }
        std::stringstream ss;
        ss << config;
        ELOGV(INFO, "config: %s", ss.str().data());
        CoroServerTester(config).run();
      }
      // }
    }
  }
}

TEST_CASE("testing coro rpc server stop") {
  ELOGV(INFO, "run testing coro rpc server stop");
  coro_rpc_server server(2, 8810);
  server.async_start().start([](auto &&) {
  });
  CHECK_MESSAGE(server.wait_for_start(3s), "server start timeout");
  SUBCASE("stop twice") {
    server.stop();
    server.stop();
  }
  SUBCASE("stop in different thread") {
    std::thread thd1([&server]() {
      server.stop();
    });
    std::thread thd2([&server]() {
      server.stop();
    });
    thd1.join();
    thd2.join();
  }
}

TEST_CASE("test server accept error") {
  ELOGV(INFO, "run test server accept error");
  g_action = inject_action::force_inject_server_accept_error;
  coro_rpc_server server(2, 8810);
  server.regist_handler<hi>();
  server.async_start().start([](auto &&) {
  });
  CHECK_MESSAGE(server.wait_for_start(3s), "server start timeout");
  coro_rpc_client client(g_client_id++);
  ELOGV(INFO, "run test server accept error, client_id %d",
        client.get_client_id());
  auto ec = syncAwait(client.connect("127.0.0.1", "8810"));
  REQUIRE_MESSAGE(ec == std::errc{},
                  std::to_string(client.get_client_id())
                      .append(make_error_code(ec).message()));
  auto ret = syncAwait(client.call<hi>());
  REQUIRE_MESSAGE(ret.error().code == std::errc::io_error, ret.error().msg);
  REQUIRE(client.has_closed() == true);

  ec = syncAwait(client.connect("127.0.0.1", "8810"));
  REQUIRE_MESSAGE(ec == std::errc::io_error,
                  std::to_string(client.get_client_id())
                      .append(make_error_code(ec).message()));
  ret = syncAwait(client.call<hi>());
  CHECK(!ret);
  REQUIRE(client.has_closed() == true);
  g_action = {};
}

TEST_CASE("test server write queue") {
  ELOGV(INFO, "run server write queue");
  g_action = {};
  coro_rpc_server server(2, 8810);
  server.regist_handler<coro_fun_with_delay_return_void_cost_long_time>();
  server.async_start().start([](auto &&) {
  });
  CHECK_MESSAGE(server.wait_for_start(3s), "server start timeout");
  std::string buffer;
  buffer.reserve(REQ_HEAD_LEN + struct_pack::get_needed_size(std::monostate{}));
  buffer.resize(REQ_HEAD_LEN);
  auto &header = *(req_header *)buffer.data();
  header.magic = magic_number;
  header.function_id =
      func_id<coro_fun_with_delay_return_void_cost_long_time>();
  header.seq_num = g_client_id++;
  header.length = struct_pack::get_needed_size(std::monostate{});
  ELOGV(INFO, "client_id %d begin to connect %d", header.seq_num, 8820);
  struct_pack::serialize_to(buffer, std::monostate{});
  asio::io_context io_context;
  std::thread thd([&io_context]() {
    asio::io_context::work work(io_context);
    io_context.run();
  });
  asio::ip::tcp::socket socket(io_context);
  auto ret = asio_util::connect(io_context, socket, "127.0.0.1", "8810");
  CHECK(!ret);
  ELOGV(INFO, "%s client_id %d call %s", "sync_client", header.seq_num,
        "coro_fun_with_delay_return_void_cost_long_time");
  for (int i = 0; i < 1; ++i) {
    auto err =
        asio_util::write(socket, asio::buffer(buffer.data(), buffer.size()));
    CHECK(err.second == buffer.size());
  }
  for (int i = 0; i < 1; ++i) {
    char buffer2[RESP_HEAD_LEN];
    std::monostate r;
    auto buf = struct_pack::serialize<std::string>(r);
    std::string buffer_read;
    buffer_read.resize(buf.size());
    read(socket, asio::buffer(buffer2, RESP_HEAD_LEN));
    auto resp_head = *(resp_header *)buffer2;
    uint32_t body_len = header.length;
    CHECK(body_len == buf.size());
    read(socket, asio::buffer(buffer_read, body_len));
    std::monostate r2;
    std::size_t sz;
    auto ret =
        struct_pack::deserialize_to(r2, buffer_read.data(), body_len, sz);
    CHECK(ret == struct_pack::errc::ok);
    CHECK(sz == body_len);
    CHECK(r2 == r);
  }

  ELOGV(INFO, "client_id %d close", header.seq_num);
  asio::error_code ignored_ec;
  socket.shutdown(asio::ip::tcp::socket::shutdown_both, ignored_ec);
  socket.close(ignored_ec);
  io_context.stop();
  thd.join();
  server.stop();
}

TEST_CASE("testing coro rpc write error") {
  ELOGV(INFO, "run testing coro rpc write error");
  g_action = inject_action::force_inject_connection_close_socket;
  coro_rpc_server server(2, 8810);
  server.regist_handler<hi>();
  server.async_start().start([](auto &&) {
  });
  CHECK_MESSAGE(server.wait_for_start(3s), "server start timeout");
  coro_rpc_client client(g_client_id++);
  ELOGV(INFO, "run testing coro rpc write error, client_id %d",
        client.get_client_id());
  auto ec = syncAwait(client.connect("127.0.0.1", "8810"));
  REQUIRE_MESSAGE(ec == std::errc{},
                  std::to_string(client.get_client_id())
                      .append(make_error_code(ec).message()));
  auto ret = syncAwait(client.call<hi>());
  REQUIRE_MESSAGE(
      ret.error().code == std::errc::io_error,
      std::to_string(client.get_client_id()).append(ret.error().msg));
  REQUIRE(client.has_closed() == true);
  g_action = inject_action::nothing;
}