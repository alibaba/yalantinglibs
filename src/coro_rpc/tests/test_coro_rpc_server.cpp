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
#include <async_simple/coro/Collect.h>
#include <async_simple/coro/SyncAwait.h>

#include <thread>
#include <variant>
#include <ylt/coro_rpc/coro_rpc_client.hpp>
#include <ylt/coro_rpc/coro_rpc_server.hpp>

#include "ServerTester.hpp"
#include "async_simple/coro/Lazy.h"
#include "doctest.h"
#include "rpc_api.hpp"
#include "ylt/coro_rpc/impl/errno.h"
#include "ylt/struct_pack.hpp"

async_simple::coro::Lazy<int> get_coro_value(int val) { co_return val; }

struct CoroServerTester : ServerTester {
  CoroServerTester(TesterConfig config)
      : ServerTester(config),
        server(2, config.port, config.conn_timeout_duration) {
#ifdef YLT_ENABLE_SSL
    if (use_ssl) {
      server.init_ssl_context(
          ssl_configure{"../openssl_files", "server.crt", "server.key"});
    }
#endif
    auto res = server.async_start();
    CHECK_MESSAGE(res, "server start timeout");
  }
  ~CoroServerTester() { server.stop(); }

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
    this->test_call_with_delay_func<
        coro_fun_with_user_define_connection_type>();
    this->test_call_with_delay_func<coro_fun_with_delay_return_void_twice>();
    this->test_call_with_delay_func_client_read_length_error<
        coro_fun_with_delay_return_void>();
    this->test_call_with_delay_func_client_read_body_error<
        coro_fun_with_delay_return_void>();
    if (enable_heartbeat) {
      this->test_call_with_delay_func_server_timeout<
          coro_fun_with_delay_return_void_cost_long_time>();
    }
    this->test_call_with_delay_func<coro_fun_with_delay_return_string>();
    this->test_call_with_delay_func<coro_fun_with_delay_return_string_twice>();
  }
  void register_all_function() override {
    g_action = {};
    ELOGV(INFO, "run %s", __func__);
    server.register_handler<async_hi, large_arg_fun, client_hello>();
    server.register_handler<long_run_func>();
    server.register_handler<&ns_login::LoginService::login>(&login_service_);
    server.register_handler<&HelloService::hello>(&hello_service_);
    server.register_handler<hello>();
    server.register_handler<coro_fun_with_user_define_connection_type>();
    server.register_handler<coro_fun_with_delay_return_void>();
    server.register_handler<coro_fun_with_delay_return_void_twice>();
    server.register_handler<coro_fun_with_delay_return_void_cost_long_time>();
    server.register_handler<coro_fun_with_delay_return_string>();
    server.register_handler<coro_fun_with_delay_return_string_twice>();
    server.register_handler<coro_func>();
    server.register_handler<coro_func_return_void>();
    server.register_handler<coro_func_delay_return_int>();
    server.register_handler<coro_func_delay_return_void>();
    server.register_handler<&HelloService::coro_func,
                            &HelloService::coro_func_return_void,
                            &HelloService::coro_func_delay_return_void,
                            &HelloService::coro_func_delay_return_int>(
        &hello_service_);
    server.register_handler<get_coro_value>();
    server.register_handler<&CoroServerTester::get_value>(this);
  }

  void test_function_not_registered() {
    g_action = {};
    auto client = create_client();
    ELOGV(INFO, "run %s, client_id %d", __func__, client->get_client_id());
    auto ret = call<function_not_registered>(client);
    REQUIRE_MESSAGE(
        ret.error().code == coro_rpc::errc::function_not_registered,
        std::to_string(client->get_client_id()).append(ret.error().msg));
    REQUIRE(client->has_closed() == true);
    ret = call<function_not_registered>(client);
    CHECK(client->has_closed() == true);
    ret = call<function_not_registered>(client);
    REQUIRE_MESSAGE(ret.error().code == coro_rpc::errc::io_error,
                    ret.error().msg);
    CHECK(client->has_closed() == true);
    server.register_handler<function_not_registered>();
  }

  void test_server_start_again() {
    ELOGV(INFO, "run %s", __func__);

    auto ec = server.start();
    REQUIRE_MESSAGE(ec == coro_rpc::errc::io_error, ec.message());
  }

  void test_start_new_server_with_same_port() {
    ELOGV(INFO, "run %s", __func__);
    {
      auto new_server = coro_rpc_server(2, std::stoi(this->port_));
      auto ec = new_server.async_start();
      REQUIRE(!ec);
      REQUIRE_MESSAGE(ec.error() == coro_rpc::errc::address_in_use,
                      ec.error().message());
    }
    ELOGV(INFO, "OH NO");
  }
  void test_server_send_bad_rpc_result() {
    auto client = create_client(inject_action::server_send_bad_rpc_result);
    ELOGV(INFO, "run %s, client_id %d", __func__, client->get_client_id());
    auto ret = this->call<hi>(client);
    CHECK_MESSAGE(
        ret.error().code == coro_rpc::errc::invalid_argument,
        std::to_string(client->get_client_id()).append(ret.error().msg));
    g_action = {};
  }

  void test_server_send_no_body() {
    auto client = create_client(inject_action::close_socket_after_send_length);
    ELOGV(INFO, "run %s, client_id %d", __func__, client->get_client_id());
    auto ret = this->template call<hello>(client);
    REQUIRE_MESSAGE(
        ret.error().code == coro_rpc::errc::io_error,
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

    auto ret4 = this->template call<coro_func_return_void>(client, 42);
    CHECK(ret4.has_value());

    auto ret5 =
        this->template call<&HelloService::coro_func_return_void>(client, 42);
    CHECK(ret5.has_value());

    auto ret6 = this->template call<&HelloService::coro_func_delay_return_void>(
        client, 42);
    CHECK(ret6.has_value());

    auto ret7 = this->template call<&HelloService::coro_func_delay_return_int>(
        client, 42);
    CHECK(ret7.value() == 42);

    auto ret8 = this->template call<coro_func_delay_return_void>(client, 42);
    CHECK(ret8.has_value());

    auto ret9 = this->template call<coro_func_delay_return_int>(client, 42);
    CHECK(ret9.value() == 42);
  }
  coro_rpc_server server;
  std::thread thd;
  HelloService hello_service_;
};
TEST_CASE("testing coro rpc server") {
  ELOGV(INFO, "run testing coro rpc server");
  unsigned short server_port = 8810;
  auto conn_timeout_duration = 500ms;
  std::vector<bool> switch_list{true, false};
  for (auto enable_heartbeat : switch_list) {
    for (auto use_ssl : switch_list) {
      TesterConfig config;
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

TEST_CASE("testing coro rpc server stop") {
  ELOGV(INFO, "run testing coro rpc server stop");
  coro_rpc_server server(2, 8810);
  auto res = server.async_start();
  REQUIRE_MESSAGE(res, "server start failed");
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
  server.register_handler<hi>();
  auto res = server.async_start();
  CHECK_MESSAGE(res, "server start timeout");
  coro_rpc_client client(*coro_io::get_global_executor(), g_client_id++);
  ELOGV(INFO, "run test server accept error, client_id %d",
        client.get_client_id());
  auto ec = syncAwait(client.connect("127.0.0.1", "8810"));
  REQUIRE_MESSAGE(!ec,
                  std::to_string(client.get_client_id()).append(ec.message()));
  auto ret = syncAwait(client.call<hi>());
  REQUIRE_MESSAGE(ret.error().code == coro_rpc::errc::io_error,
                  ret.error().msg);
  REQUIRE(client.has_closed() == true);

  ec = syncAwait(client.connect("127.0.0.1", "8810"));
  REQUIRE_MESSAGE(ec == coro_rpc::errc::io_error,
                  std::to_string(client.get_client_id()).append(ec.message()));
  ret = syncAwait(client.call<hi>());
  CHECK(!ret);
  REQUIRE(client.has_closed() == true);
  g_action = {};
}

TEST_CASE("test server write queue") {
  using coro_rpc_protocol = coro_rpc::protocol::coro_rpc_protocol;
  ELOGV(INFO, "run server write queue");
  g_action = {};
  coro_rpc_server server(2, 8810);
  server.register_handler<coro_fun_with_delay_return_void_cost_long_time>();
  auto res = server.async_start();
  CHECK_MESSAGE(res, "server start timeout");
  std::string buffer;
  buffer.reserve(coro_rpc_protocol::REQ_HEAD_LEN +
                 struct_pack::get_needed_size(std::monostate{}));
  buffer.resize(coro_rpc_protocol::REQ_HEAD_LEN);
  auto &header = *(coro_rpc_protocol::req_header *)buffer.data();
  header.magic = coro_rpc_protocol::magic_number;
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
  auto ret = coro_io::connect(io_context, socket, "127.0.0.1", "8810");
  CHECK(!ret);
  ELOGV(INFO, "%s client_id %d call %s", "sync_client", header.seq_num,
        "coro_fun_with_delay_return_void_cost_long_time");
  for (int i = 0; i < 1; ++i) {
    auto err =
        coro_io::write(socket, asio::buffer(buffer.data(), buffer.size()));
    CHECK(err.second == buffer.size());
  }
  for (int i = 0; i < 1; ++i) {
    char buffer2[coro_rpc_protocol::RESP_HEAD_LEN];
    std::monostate r;
    auto buf = struct_pack::serialize<std::string>(r);
    std::string buffer_read;
    buffer_read.resize(buf.size());
    read(socket, asio::buffer(buffer2, coro_rpc_protocol::RESP_HEAD_LEN));
    [[maybe_unused]] auto resp_head =
        *(coro_rpc_protocol::resp_header *)buffer2;
    uint32_t body_len = header.length;
    CHECK(body_len == buf.size());
    read(socket, asio::buffer(buffer_read, body_len));
    std::monostate r2;
    std::size_t sz;
    auto ret =
        struct_pack::deserialize_to(r2, buffer_read.data(), body_len, sz);
    CHECK(!ret);
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
  server.register_handler<hi>();
  auto res = server.async_start();
  CHECK_MESSAGE(res, "server start failed");
  coro_rpc_client client(*coro_io::get_global_executor(), g_client_id++);
  ELOGV(INFO, "run testing coro rpc write error, client_id %d",
        client.get_client_id());
  auto ec = syncAwait(client.connect("127.0.0.1", "8810"));
  REQUIRE_MESSAGE(!ec,
                  std::to_string(client.get_client_id()).append(ec.message()));
  auto ret = syncAwait(client.call<hi>());
  REQUIRE_MESSAGE(
      ret.error().code == coro_rpc::errc::io_error,
      std::to_string(client.get_client_id()).append(ret.error().msg));
  REQUIRE(client.has_closed() == true);
  g_action = inject_action::nothing;
}