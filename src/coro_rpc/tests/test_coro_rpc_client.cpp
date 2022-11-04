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
#include <async_simple/coro/Lazy.h>
#include <async_simple/coro/SyncAwait.h>

#include <chrono>
#include <coro_rpc/coro_rpc/async_rpc_server.hpp>
#include <coro_rpc/coro_rpc_client.hpp>
#include <coro_rpc/coro_rpc_server.hpp>
#include <cstddef>
#include <memory>
#include <thread>
#include <variant>

#include "asio/io_context.hpp"
#include "asio_util/asio_coro_util.hpp"
#include "coro_rpc/coro_rpc/rpc_protocol.h"
#include "doctest.h"
#include "rpc_api.hpp"
#include "struct_pack/struct_pack.hpp"
using namespace coro_rpc;
using namespace std::chrono_literals;
using namespace std::string_literals;
using namespace async_simple::coro;
using asio::ip::tcp;

template <typename Server>
class ClientTester {
  ClientTester(unsigned short port) : port_(std::to_string(port)) {}
  std::string port_;
};

static unsigned short coro_rpc_server_port = 8803;
Lazy<std::shared_ptr<coro_rpc_client>> create_client(
    asio::io_context& io_context, std::string port) {
  auto client = std::make_shared<coro_rpc_client>(io_context);
#ifdef ENABLE_SSL
  bool ok = client->init_ssl("../openssl_files", "server.crt");
  REQUIRE_MESSAGE(ok == true, "init ssl fail, please check ssl config");
#endif
  auto ec = co_await client->connect("127.0.0.1", port);
  REQUIRE_MESSAGE(ec == err_ok, make_error_code(ec).message());
  co_return client;
}

TEST_CASE("testing client") {
  std::string port = std::to_string(coro_rpc_server_port);
  asio::io_context io_context;
  std::promise<void> promise;
  auto future = promise.get_future();
  std::thread thd([&io_context, &promise] {
    asio::io_context::work work(io_context);
    promise.set_value();
    io_context.run();
  });

  future.wait();
  coro_rpc_server server(2, coro_rpc_server_port);
#ifdef ENABLE_SSL
  server.init_ssl_context(
      ssl_configure{"../openssl_files", "server.crt", "server.key"});
#endif
  server.async_start().start([](auto&&) {
  });

  CHECK_MESSAGE(server.wait_for_start(3s), "server start timeout");

  SUBCASE("call rpc, function not registered") {
    auto f = [&io_context, &port]() -> Lazy<void> {
      auto client = co_await create_client(io_context, port);
      auto ret = co_await client->template call<hello>();
      CHECK_MESSAGE(ret.error().code == std::errc::function_not_supported,
                    ret.error().msg);
      co_return;
    };
    syncAwait(f());
  }
  SUBCASE("call rpc, function not registered 2") {
    auto f = [&io_context, &port]() -> Lazy<void> {
      auto client = co_await create_client(io_context, port);
      auto ret = co_await client->template call<hi>();
      CHECK_MESSAGE(ret.error().code == std::errc::function_not_supported,
                    ret.error().msg);
      co_return;
    };
    syncAwait(f());
  }

  SUBCASE("call rpc timeout") {
    auto f = [&io_context, &port]() -> Lazy<void> {
      auto client = co_await create_client(io_context, port);
      auto ret = co_await client->template call_for<hello_timeout>(30ms);
      CHECK_MESSAGE(ret.error().code == std::errc::timed_out, ret.error().msg);
      co_return;
    };
    register_handler<hello_timeout>();
    syncAwait(f());
    remove_handler<hello_timeout>();
  }

  SUBCASE("call rpc success") {
    auto f = [&io_context, &port]() -> Lazy<void> {
      auto client = co_await create_client(io_context, port);
      auto ret = co_await client->template call<hello>();
      CHECK(ret.value() == std::string("hello"));
      ret = co_await client->call_for<hello>(100ms);
      CHECK(ret.value() == std::string("hello"));
      co_return;
    };
    register_handler<hello>();
    syncAwait(f());
    remove_handler<hello>();
  }

  SUBCASE("call with large buffer") {
    auto f = [&io_context, &port]() -> Lazy<void> {
      auto client = co_await create_client(io_context, port);
      std::string arg;
      arg.resize(2048);
      auto ret = co_await client->template call<large_arg_fun>(arg);
      CHECK(ret.value() == arg);
      co_return;
    };
    register_handler<large_arg_fun>();
    syncAwait(f());
    remove_handler<large_arg_fun>();
  }

  server.stop();
  io_context.stop();
  thd.join();
}

TEST_CASE("testing client with inject server") {
  std::string port = std::to_string(coro_rpc_server_port);
  easylog::info("inject server port: {}", port);
  asio::io_context io_context;
  std::thread thd([&io_context] {
    asio::io_context::work work(io_context);
    io_context.run();
  });
  coro_rpc_server server(2, coro_rpc_server_port);
#ifdef ENABLE_SSL
  server.init_ssl_context(
      ssl_configure{"../openssl_files", "server.crt", "server.key"});
#endif
  server.async_start().start([](auto&&) {
  });
  CHECK_MESSAGE(server.wait_for_start(3s), "server start timeout");

  register_handler<hello>();

  SUBCASE("server run ok") {
    auto f = [&io_context, &port]() -> Lazy<void> {
      auto client = co_await create_client(io_context, port);
      auto ret = co_await client->template call<hello>();
      CHECK(ret.value() == std::string("hello"));
      co_return;
    };
    syncAwait(f());
  }

  SUBCASE("client read length error") {
    auto f = [&io_context, &port]() -> Lazy<void> {
      auto client = co_await create_client(io_context, port);
      g_action = inject_action::close_socket_after_read_header;
      auto ret = co_await client->template call<hello>();
      REQUIRE_MESSAGE(ret.error().code == std::errc::io_error, ret.error().msg);
    };
    syncAwait(f());
  }
  SUBCASE("client read body error") {
    auto f = [&io_context, &port]() -> Lazy<void> {
      auto client = co_await create_client(io_context, port);
      g_action = inject_action::close_socket_after_send_length;
      auto ret = co_await client->template call<hello>();
      REQUIRE_MESSAGE(ret.error().code == std::errc::io_error, ret.error().msg);
    };
    syncAwait(f());
  }

  remove_handler<hello>();

  server.stop();
  io_context.stop();
  thd.join();
  g_action = inject_action::nothing;
}
#ifdef ENABLE_SSL

enum class ssl_type { _, fake, no };

template <typename Server>
class SSLClientTester {
 public:
  SSLClientTester(std::string base_path, unsigned short port,
                  ssl_type client_crt, ssl_type server_crt, ssl_type server_key,
                  ssl_type dh)
      : port_(std::to_string(port)),
        server(2, port),
        base_path(base_path),
        client_crt(client_crt),
        server_crt(server_crt),
        server_key(server_key),
        dh(dh) {
    register_handler<hi>();
    inject("client crt", client_crt_path, client_crt);
    inject("server crt", server_crt_path, server_crt);
    inject("server key", server_key_path, server_key);
    inject("dh", dh_path, dh);
    ssl_configure config{base_path, server_crt_path, server_key_path, dh_path};
    server.init_ssl_context(config);
    if constexpr (std::is_same_v<Server, coro_rpc_server>) {
      server.async_start().start([](auto&&) {
      });
      CHECK_MESSAGE(server.wait_for_start(3s), "server start timeout");
    }
    else if constexpr (std::is_same_v<Server, async_rpc_server>) {
      server.async_start();
      CHECK_MESSAGE(server.wait_for_start(3s), "server start timeout");
    }

    std::promise<void> promise;
    auto future = promise.get_future();
    thd = std::thread([this, &promise] {
      asio::io_context::work work(io_context);
      promise.set_value();
      io_context.run();
    });
    future.wait();
  }
  ~SSLClientTester() {
    remove_handler<hi>();

    io_context.stop();
    thd.join();
  }
  void inject(std::string msg, std::string& path, ssl_type type) {
    switch (type) {
      case ssl_type::fake: {
        path = "fake_" + path;
        break;
      }
      case ssl_type::no: {
        path = "no_" + path;
        break;
      }
      default: {
      }
    }
    easylog::info("{}: {}", msg, path);
  }
  void run() {
    auto client = std::make_shared<coro_rpc_client>(io_context);
    bool ok = client->init_ssl(base_path, client_crt_path);
    if (client_crt == ssl_type::fake || client_crt == ssl_type::no) {
      REQUIRE(ok == false);
      auto ec = syncAwait(client->connect("127.0.0.1", port_));
      REQUIRE_MESSAGE(ec == std::errc::not_connected,
                      make_error_code(ec).message());
      auto ret = syncAwait(client->template call<hi>());
      REQUIRE_MESSAGE(ret.error().code == std::errc::not_connected,
                      ret.error().msg);
    }
    else {
      REQUIRE_MESSAGE(ok == true, "init ssl fail, please check ssl config");
      auto f = [this, &client]() -> Lazy<void> {
        auto ec = co_await client->connect("127.0.0.1", port_);
        if (server_crt == ssl_type::_ && server_key == ssl_type::_) {
          if (ec != std::errc{}) {
            easylog::error("{}", gen_err());
          }
          REQUIRE_MESSAGE(ec == std::errc{}, make_error_code(ec).message());
          auto ret = co_await client->template call<hi>();
          CHECK(ret.has_value());
        }
        else {
          REQUIRE_MESSAGE(ec == std::errc::not_connected,
                          make_error_code(ec).message());
        }
      };
      syncAwait(f());
    }
  }

  std::string gen_err() {
    auto to_string = [](ssl_type t) -> std::string {
      switch (t) {
        case ssl_type::fake: {
          return "fake"s;
        }
        case ssl_type::no: {
          return "no"s;
        }
        default: {
          return "_"s;
        }
      }
    };
    std::stringstream buf;
    buf << "\n";
    buf << "client crt: " << client_crt_path << " __ " << to_string(client_crt)
        << "; ";
    buf << "server crt: " << server_crt_path << " __ " << to_string(server_crt)
        << "; ";
    buf << "server key: " << server_key_path << " __ " << to_string(server_key)
        << "; ";
    buf << "dh: " << dh_path << " __ " << to_string(dh) << "; ";
    return buf.str();
  }

  std::string port_;
  Server server;
  std::string base_path;
  std::string client_crt_path = "server.crt";
  std::string server_crt_path = "server.crt";
  std::string server_key_path = "server.key";
  std::string dh_path = "dhparam.pem";
  ssl_type client_crt;
  ssl_type server_crt;
  ssl_type server_key;
  ssl_type dh;
  asio::io_context io_context;
  std::thread thd;
};

TEST_CASE("testing client with ssl server") {
  std::vector<ssl_type> type_list{ssl_type::fake, ssl_type::no, ssl_type::_};
  std::string base_path = "../openssl_files";
  unsigned short port = 8809;
  for (auto client_crt : type_list) {
    for (auto server_crt : type_list) {
      for (auto server_key : type_list) {
        for (auto dh : type_list) {
          SSLClientTester<coro_rpc_server>(base_path, port, client_crt,
                                           server_crt, server_key, dh)
              .run();
          SSLClientTester<async_rpc_server>(base_path, port, client_crt,
                                            server_crt, server_key, dh)
              .run();
        }
      }
    }
  }
}
#endif
TEST_CASE("testing client with eof") {
  coro_rpc_server server(2, 8801);
  server.async_start().start([](auto&&) {
  });
  CHECK_MESSAGE(server.wait_for_start(3s), "server start timeout");
  coro_rpc_client client;
  auto ec = client.sync_connect("127.0.0.1", "8801");
  REQUIRE_MESSAGE(ec == std::errc{}, make_error_code(ec).message());

  register_handler<hello, client_hello>();
  auto ret = client.sync_call<hello>();
  CHECK(ret.value() == "hello"s);

  ret = client.sync_call<client_hello>();
  CHECK(ret.value() == "client hello"s);

  g_action = inject_action::client_close_socket_after_send_partial_header;
  ret = client.sync_call<hello>();
  REQUIRE_MESSAGE(ret.error().code == std::errc::io_error, ret.error().msg);

  remove_handler<hello>();
  remove_handler<client_hello>();
}

TEST_CASE("testing client with shutdown") {
  coro_rpc_server server(2, 8801);
  server.async_start().start([](auto&&) {
  });
  CHECK_MESSAGE(server.wait_for_start(3s), "server start timeout");
  coro_rpc_client client;
  auto ec = client.sync_connect("127.0.0.1", "8801");
  REQUIRE_MESSAGE(ec == std::errc{}, make_error_code(ec).message());
  register_handler<hello, client_hello>();

  g_action = inject_action::nothing;
  auto ret = client.sync_call<hello>();
  CHECK(ret.value() == "hello"s);

  ret = client.sync_call<client_hello>();
  CHECK(ret.value() == "client hello"s);

  g_action = inject_action::client_shutdown_socket_after_send_header;
  ret = client.sync_call<hello>();
  REQUIRE_MESSAGE(ret.error().code == std::errc::io_error, ret.error().msg);

  remove_handler<hello>();
  remove_handler<client_hello>();
}
TEST_CASE("testing client timeout") {
  // SUBCASE("connect, 0ms timeout") {
  //   coro_rpc_client client;
  //   auto ret = client.connect("127.0.0.1", "8801", 0ms);
  //   auto val = syncAwait(ret);

  //   CHECK_MESSAGE(val == std::errc::timed_out,
  //   make_error_code(val).message());
  // }
  SUBCASE("connect, ip timeout") {
    // https://stackoverflow.com/questions/100841/artificially-create-a-connection-timeout-error
    coro_rpc_client client;
    auto ret = client.connect("10.255.255.1", "8801", 5ms);
    auto val = syncAwait(ret);
    CHECK_MESSAGE(val == std::errc::timed_out, make_error_code(val).message());
  }
  // SUBCASE("call, 0ms timeout") {
  //   coro_rpc_server server(2, 8801);
  //   server.async_start().start([](auto&&) {
  //   });
  //   coro_rpc_client client;
  //   auto ec_lazy = client.connect("127.0.0.1", "8801", 5ms);
  //   auto ec = syncAwait(ec_lazy);
  //   assert(ec == std::errc{});
  //   auto ret = client.call_for<hi>(0ms);
  //   auto val = syncAwait(ret);
  //   CHECK_MESSAGE(val.error().code == std::errc::timed_out, val.error().msg);
  // }
}
TEST_CASE("testing client connect err") {
  coro_rpc_client client;
  auto val = syncAwait(client.connect("127.0.0.1", "8801"));
  CHECK_MESSAGE(val == std::errc::not_connected,
                make_error_code(val).message());
}
#ifdef UNIT_TEST_INJECT
TEST_CASE("testing client sync connect, unit test inject only") {
  coro_rpc_client client;
  auto val = client.sync_connect("127.0.0.1", "8801");
  CHECK_MESSAGE(val == std::errc::not_connected,
                make_error_code(val).message());
#ifdef ENABLE_SSL
  SUBCASE("client use ssl but server don't use ssl") {
    coro_rpc_server server(2, 8801);
    server.async_start().start([](auto&&) {
    });
    CHECK_MESSAGE(server.wait_for_start(3s), "server start timeout");
    coro_rpc_client client2;
    bool ok = client2.init_ssl("../openssl_files", "server.crt");
    CHECK(ok == true);
    val = client2.sync_connect("127.0.0.1", "8801");
    CHECK_MESSAGE(val == std::errc::not_connected,
                  make_error_code(val).message());
  }
#endif
}
#endif
TEST_CASE("testing client call timeout") {
  register_handler<hello_timeout>();
  register_handler<timeout_due_to_heartbeat>();
  register_handler<hi>();
  SUBCASE("write timeout") {
    g_action = inject_action::force_inject_client_write_data_timeout;
    //    coro_rpc_server server(2, 8801);
    //    server.async_start().start([](auto&&) {
    //    });
    coro_rpc_client client;
    //    auto ec_lazy = client.connect("127.0.0.1", "8801", 5ms);
    //    auto ec = syncAwait(ec_lazy);
    //    assert(ec == std::errc{});
    auto ret = client.call<hi>();
    auto val = syncAwait(ret);
    CHECK_MESSAGE(val.error().code == std::errc::timed_out, val.error().msg);
    g_action = inject_action::nothing;
  }
  SUBCASE("read timeout") {
    coro_rpc_server server(2, 8801);
    server.async_start().start([](auto&&) {
    });
    CHECK_MESSAGE(server.wait_for_start(3s), "server start timeout");
    coro_rpc_client client;
    auto ec_lazy = client.connect("127.0.0.1", "8801", 5ms);
    auto ec = syncAwait(ec_lazy);
    REQUIRE(ec == std::errc{});
    auto ret = client.call_for<hello_timeout>(10ms);
    auto val = syncAwait(ret);
    CHECK_MESSAGE(val.error().code == std::errc::timed_out, val.error().msg);
  }
  remove_handler<hello_timeout>();
  remove_handler<timeout_due_to_heartbeat>();
  remove_handler<hi>();
}
std::errc init_acceptor(auto& acceptor_, auto port_) {
  using asio::ip::tcp;
  auto endpoint = tcp::endpoint(tcp::v4(), port_);
  acceptor_.open(endpoint.protocol());
  acceptor_.set_option(tcp::acceptor::reuse_address(true));

  asio::error_code ec;
  acceptor_.bind(endpoint, ec);
  if (ec) {
    easylog::error("bind error : {}", ec.message());
    acceptor_.cancel(ec);
    acceptor_.close(ec);
    return std::errc::address_in_use;
  }
  acceptor_.listen();

  easylog::info("listen port {} successfully", port_);
  return std::errc{};
}
// TODO: will open after code refactor.
//  TEST_CASE("testing read body timeout") {
//    register_handler<hello>();
//    std::promise<void> server_p;
//    std::promise<void> p;
//    std::thread thd = std::thread([&server_p, &p]() {
//      asio::io_context io_context;
//      tcp::acceptor acceptor(io_context);
//      tcp::socket socket(io_context);
//      auto ec = init_acceptor(acceptor, 8801);
//      REQUIRE(ec == std::errc{});
//      easylog::info("server started");
//      server_p.set_value();
//      auto ec2 = accept(acceptor, socket);
//      REQUIRE(!ec2);
//      std::array<std::byte, RPC_HEAD_LEN> head;
//      read(socket, asio::buffer(head, RPC_HEAD_LEN));
//      rpc_header header;
//      auto errc = struct_pack::deserialize_to(header, head);
//      REQUIRE(errc == std::errc{});
//      std::vector<std::byte> body_;
//      body_.resize(header.length);
//      auto ret = read(socket, asio::buffer(body_.data(), header.length));
//      REQUIRE(!ret.first);
//      auto buf = struct_pack::serialize_with_offset(
//          /*offset = */ RESPONSE_HEADER_LEN, std::monostate{});
//      *((uint32_t*)buf.data()) = buf.size() - RESPONSE_HEADER_LEN;
//      write(socket, asio::buffer(buf.data(), RESPONSE_HEADER_LEN));
//      std::this_thread::sleep_for(50ms);
//      write(socket, asio::buffer(buf.data() + RESPONSE_HEADER_LEN,
//                                 buf.size() - RESPONSE_HEADER_LEN));
//      p.set_value();
//    });
//    easylog::info("wait for server start");
//    server_p.get_future().wait();
//    easylog::info("custom server started");
//    coro_rpc_client client;
//    auto ec_lazy = client.connect("127.0.0.1", "8801"s, 5ms);
//    auto ec = syncAwait(ec_lazy);
//    REQUIRE(ec == std::errc{});
//    auto ret = client.call_for<hello>(50ms);
//    auto val = syncAwait(ret);
//    CHECK_MESSAGE(val.error().code == std::errc::timed_out, val.error().msg);
//    remove_handler<hello>();
//    thd.join();
//    p.get_future().wait();
//  }
