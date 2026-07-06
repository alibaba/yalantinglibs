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

// Test for SSL use-after-free fix in coro_rpc_client::connect_impl
//
// Bug: When an SSL client in a connection pool times out and reconnects,
// connect_impl calls reset() which destroys and recreates ssl_stream_.
// The Socket& soc parameter captured before reset() now dangles,
// and async_connect(soc, *eps) accesses freed memory → 0xC0000005 crash.
//
// Fix: Use socket_wrapper_.visit() to get a fresh socket reference
// for async_connect() instead of the potentially-stale soc parameter.

#include <async_simple/coro/SyncAwait.h>

#include <asio/io_context.hpp>
#include <iostream>
#include <thread>
#include <ylt/coro_io/client_pool.hpp>
#include <ylt/coro_rpc/coro_rpc_client.hpp>
#include <ylt/coro_rpc/coro_rpc_server.hpp>

#include "doctest.h"

using namespace coro_rpc;
using namespace async_simple::coro;

const std::string CERT_PATH = "../openssl_files";
const std::string SERVER_CERT = "mutual_server.crt";
const std::string SERVER_KEY = "mutual_server.key";
const std::string CLIENT_CERT = "mutual_client.crt";
const std::string CLIENT_KEY = "mutual_client.key";
const std::string CA_CERT = "mutual_ca.crt";

namespace {
std::string hello_ssl_test() { return "Hello SSL Reconnect"; }
}  // namespace

#ifdef YLT_ENABLE_SSL

TEST_CASE("testing SSL client pool reconnect after close") {
  coro_rpc_server server(2, 8821);

  ssl_configure ssl_conf;
  ssl_conf.base_path = CERT_PATH;
  ssl_conf.cert_file = SERVER_CERT;
  ssl_conf.key_file = SERVER_KEY;
  ssl_conf.ca_cert_file = "";
  ssl_conf.enable_client_verify = false;

  server.init_ssl(ssl_conf);
  server.register_handler<hello_ssl_test>();

  auto res = server.async_start();
  REQUIRE_MESSAGE(!res.hasResult(), "server start failed");

  coro_rpc_client::config client_config;
  coro_rpc_client::tcp_with_ssl_config ssl_cfg;
  ssl_cfg.ssl_cert_path = std::filesystem::path(CERT_PATH) / CA_CERT;
  ssl_cfg.ssl_domain = "127.0.0.1";
  ssl_cfg.enable_tcp_no_delay = true;
  client_config.socket_config = ssl_cfg;

  auto pool = coro_io::client_pool<coro_rpc_client>::create(
      "127.0.0.1:8821",
      {.max_connection = 10,
       .connect_retry_count = 3,
       .idle_timeout = std::chrono::milliseconds(30000),
       .client_config = client_config});

  // First request: establish SSL connection
  auto result = syncAwait(pool->send_request(
      [](coro_rpc_client &client) {
        return client.call<hello_ssl_test>();
      }));
  REQUIRE_MESSAGE(result.has_value(), "first call should succeed");
  CHECK(result.value() == "Hello SSL Reconnect");

  // Close the client to force reconnect on next request.
  // close() is synchronous (void), call<>() returns Lazy — no coroutine needed.
  syncAwait(pool->send_request(
      [](coro_rpc_client &client) {
        client.close();
        return client.call<hello_ssl_test>();
      }));

  // Second request: reconnect path triggers reset() -> init_ssl()
  // Without the fix, the dangling Socket& in connect_impl would cause
  // an access violation (0xC0000005) on Windows IOCP
  auto result2 = syncAwait(pool->send_request(
      [](coro_rpc_client &client) {
        return client.call<hello_ssl_test>();
      }));
  REQUIRE_MESSAGE(result2.has_value(), "reconnect call should succeed");
  CHECK(result2.value() == "Hello SSL Reconnect");

  server.stop();
}

TEST_CASE("testing SSL client pool multiple reconnect cycles") {
  coro_rpc_server server(2, 8822);

  ssl_configure ssl_conf;
  ssl_conf.base_path = CERT_PATH;
  ssl_conf.cert_file = SERVER_CERT;
  ssl_conf.key_file = SERVER_KEY;
  ssl_conf.ca_cert_file = "";
  ssl_conf.enable_client_verify = false;

  server.init_ssl(ssl_conf);
  server.register_handler<hello_ssl_test>();

  auto res = server.async_start();
  REQUIRE_MESSAGE(!res.hasResult(), "server start failed");

  coro_rpc_client::config client_config;
  coro_rpc_client::tcp_with_ssl_config ssl_cfg;
  ssl_cfg.ssl_cert_path = std::filesystem::path(CERT_PATH) / CA_CERT;
  ssl_cfg.ssl_domain = "127.0.0.1";
  ssl_cfg.enable_tcp_no_delay = true;
  client_config.socket_config = ssl_cfg;

  auto pool = coro_io::client_pool<coro_rpc_client>::create(
      "127.0.0.1:8822",
      {.max_connection = 5,
       .connect_retry_count = 3,
       .idle_timeout = std::chrono::milliseconds(30000),
       .client_config = client_config});

  for (int i = 0; i < 5; ++i) {
    auto result = syncAwait(pool->send_request(
        [](coro_rpc_client &client) {
          return client.call<hello_ssl_test>();
        }));
    REQUIRE_MESSAGE(result.has_value(), "call should succeed");
    CHECK(result.value() == "Hello SSL Reconnect");

    syncAwait(pool->send_request(
        [](coro_rpc_client &client) {
          client.close();
          return client.call<hello_ssl_test>();
        }));
  }

  server.stop();
}

TEST_CASE("testing SSL client direct reconnect") {
  coro_rpc_server server(2, 8823);

  ssl_configure ssl_conf;
  ssl_conf.base_path = CERT_PATH;
  ssl_conf.cert_file = SERVER_CERT;
  ssl_conf.key_file = SERVER_KEY;
  ssl_conf.ca_cert_file = "";
  ssl_conf.enable_client_verify = false;

  server.init_ssl(ssl_conf);
  server.register_handler<hello_ssl_test>();

  asio::io_context io_context;
  std::thread thd([&io_context]() {
    asio::io_context::work work(io_context);
    io_context.run();
  });

  auto res = server.async_start();
  REQUIRE_MESSAGE(!res.hasResult(), "server start failed");

  coro_rpc_client client;
  bool init_ok = client.init_ssl(CERT_PATH, CA_CERT, "127.0.0.1");
  REQUIRE_MESSAGE(init_ok == true, "init ssl fail");

  auto ec = syncAwait(client.connect("127.0.0.1", "8823"));
  REQUIRE_MESSAGE(!ec, "first connect failed");

  auto result = syncAwait(client.call<hello_ssl_test>());
  REQUIRE_MESSAGE(result, "first call failed");
  CHECK(result.value() == "Hello SSL Reconnect");

  client.close();

  ec = syncAwait(client.connect("127.0.0.1", "8823"));
  REQUIRE_MESSAGE(!ec, "reconnect failed");

  result = syncAwait(client.call<hello_ssl_test>());
  REQUIRE_MESSAGE(result, "reconnect call failed");
  CHECK(result.value() == "Hello SSL Reconnect");

  server.stop();
  io_context.stop();
  thd.join();
}
#endif
