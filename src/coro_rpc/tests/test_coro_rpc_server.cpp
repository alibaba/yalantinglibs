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
#include "coro_rpc/coro_rpc/remote.hpp"
#include "doctest.h"
#include "rpc_api.hpp"

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
    easylog::info("run {}", __func__);
    test_coro_handler();
    ServerTester::test_all();
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
    easylog::info("run {}", __func__);
    ServerTester::register_all_function();
    register_handler<coro_fun_with_delay_return_void>();
    register_handler<coro_fun_with_delay_return_void_twice>();
    register_handler<coro_fun_with_delay_return_void_cost_long_time>();
    register_handler<coro_fun_with_delay_return_string>();
    register_handler<coro_fun_with_delay_return_string_twice>();
    register_handler<coro_func>();
    register_handler<&HelloService::coro_func>(&hello_service_);
    register_handler<get_coro_value>();
    register_handler<&CoroServerTester::get_value>(this);
  }
  void remove_all_rpc_function() override {
    easylog::info("run {}", __func__);
    test_server_start_again();
    ServerTester::remove_all_rpc_function();
    remove_handler<coro_fun_with_delay_return_void>();
    remove_handler<coro_fun_with_delay_return_void_twice>();
    remove_handler<coro_fun_with_delay_return_void_cost_long_time>();
    remove_handler<coro_fun_with_delay_return_string>();
    remove_handler<coro_fun_with_delay_return_string_twice>();
    remove_handler<coro_func>();
    remove_handler<&HelloService::coro_func>();
    remove_handler<get_coro_value>();
    remove_handler<&CoroServerTester::get_value>();
  }

  void test_server_start_again() {
    easylog::info("run {}", __func__);
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
    easylog::info("run {}", __func__);
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
    easylog::info("run {}", __func__);
    auto client = create_client(inject_action::server_send_bad_rpc_result);
    auto ret = this->call<hi>(client);
    CHECK_MESSAGE(
        ret.error().code == std::errc::invalid_argument,
        std::to_string(client->get_client_id()).append(ret.error().msg));
    g_action = {};
  }

  void test_server_send_no_body() {
    auto client = create_client(inject_action::close_socket_after_send_length);
    auto ret = this->template call<hello>(client);
    REQUIRE_MESSAGE(
        ret.error().code == std::errc::io_error,
        std::to_string(client->get_client_id()).append(ret.error().msg));
    g_action = {};
  }

  void test_coro_handler() {
    easylog::info("run {}", __func__);
    auto client = create_client(inject_action::nothing);
    auto ret = this->template call<get_coro_value>(client, 42);
    CHECK(ret.value() == 42);

    auto ret1 = this->template call<&HelloService::coro_func>(client, 42);
    CHECK(ret1.value() == 42);

    auto ret2 = this->template call<&CoroServerTester::get_value>(client, 42);
    CHECK(ret2.value() == 42);

    auto ret3 = this->template call<coro_func>(client, 42);
    CHECK(ret3.value() == 42);
  }
  coro_rpc_server server;
  std::thread thd;
  HelloService hello_service_;
};
TEST_CASE("testing coro rpc server") {
  unsigned short server_port = 8810;
  auto conn_timeout_duration = 300ms;
  std::vector<bool> switch_list{true, false};
  for (auto async_start : switch_list) {
    for (auto enable_heartbeat : switch_list) {
      // for (auto sync_client : switch_list) {
      for (auto use_outer_io_context : switch_list) {
        for (auto use_ssl : switch_list) {
          TesterConfig config;
          config.async_start = async_start;
          config.enable_heartbeat = enable_heartbeat;
          config.use_ssl = use_ssl;
          config.sync_client = false;
          config.use_outer_io_context = use_outer_io_context;
          config.port = server_port;
          if (enable_heartbeat) {
            config.conn_timeout_duration = conn_timeout_duration;
          }
          std::stringstream ss;
          ss << config;
          easylog::info("config: {}", ss.str());
          CoroServerTester(config).run();
        }
      }
      // }
    }
  }
}

TEST_CASE("testing coro rpc server stop") {
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
  register_handler<hi>();
  g_action = inject_action::force_inject_server_accept_error;
  coro_rpc_server server(2, 8810);
  server.async_start().start([](auto &&) {
  });
  CHECK_MESSAGE(server.wait_for_start(3s), "server start timeout");
  coro_rpc_client client(g_client_id++);
  auto ec = syncAwait(client.connect("127.0.0.1", "8810"));
  REQUIRE_MESSAGE(ec == std::errc{},
                  std::to_string(client.get_client_id())
                      .append(make_error_code(ec).message()));
  auto ret = syncAwait(client.call<hi>());
  REQUIRE_MESSAGE(ret.error().code == std::errc::io_error, ret.error().msg);
  REQUIRE(client.has_closed() == true);

  ec = syncAwait(client.connect("127.0.0.1", "8810"));
  REQUIRE_MESSAGE(ec == std::errc{},
                  std::to_string(client.get_client_id())
                      .append(make_error_code(ec).message()));
  ret = syncAwait(client.call<hi>());
  CHECK(ret.has_value());
  REQUIRE(client.has_closed() == false);
  remove_handler<hi>();
  g_action = {};
}

TEST_CASE("test server write queue") {
  g_action = {};
  remove_handler<coro_fun_with_delay_return_void_cost_long_time>();
  register_handler<coro_fun_with_delay_return_void_cost_long_time>();
  coro_rpc_server server(2, 8810);
  server.async_start().start([](auto &&) {
  });
  CHECK_MESSAGE(server.wait_for_start(3s), "server start timeout");
  constexpr auto id = func_id<coro_fun_with_delay_return_void_cost_long_time>();
  std::size_t offset = RPC_HEAD_LEN + FUNCTION_ID_LEN;
  std::vector<std::byte> buffer;
  buffer.resize(offset);
  std::memcpy(buffer.data() + RPC_HEAD_LEN, &id, FUNCTION_ID_LEN);
  rpc_header header{magic_number};
  header.seq_num = g_client_id++;
  header.length = buffer.size() - RPC_HEAD_LEN;
  auto sz = struct_pack::serialize_to(buffer.data(), RPC_HEAD_LEN, header);
  CHECK(sz == RPC_HEAD_LEN);
  asio::io_context io_context;
  std::thread thd([&io_context]() {
    asio::io_context::work work(io_context);
    io_context.run();
  });
  asio::ip::tcp::socket socket(io_context);
  auto ret = connect(io_context, socket, "127.0.0.1", "8810");
  CHECK(!ret);
  for (int i = 0; i < 10; ++i) {
    auto err = write(socket, asio::buffer(buffer.data(), buffer.size()));
    CHECK(err.second == buffer.size());
  }
  for (int i = 0; i < 10; ++i) {
    std::byte resp_len_buf[RESPONSE_HEADER_LEN];
    std::monostate r;
    auto buf = struct_pack::serialize<std::string>(r);
    std::string buffer_read;
    buffer_read.resize(buf.size());
    read(socket, asio::buffer(resp_len_buf, RESPONSE_HEADER_LEN));
    uint32_t body_len = *(uint32_t *)resp_len_buf;
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
  asio::error_code ignored_ec;
  socket.shutdown(asio::ip::tcp::socket::shutdown_both, ignored_ec);
  socket.close(ignored_ec);
  io_context.stop();
  thd.join();
  server.stop();
  remove_handler<coro_fun_with_delay_return_void_cost_long_time>();
}

TEST_CASE("testing coro rpc write error") {
  register_handler<hi>();
  g_action = inject_action::force_inject_connection_close_socket;
  coro_rpc_server server(2, 8810);
  server.async_start().start([](auto &&) {
  });
  CHECK_MESSAGE(server.wait_for_start(3s), "server start timeout");
  coro_rpc_client client(g_client_id++);
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
  remove_handler<hi>();
}