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

#include <async_simple/coro/SyncAwait.h>

#include <asio/io_context.hpp>
#include <cinatra/coro_http_client.hpp>
#include <iostream>
#include <thread>
#include <ylt/coro_rpc/coro_rpc_client.hpp>
#include <ylt/coro_rpc/coro_rpc_server.hpp>

using namespace coro_rpc;
using namespace async_simple::coro;

const std::string CERT_PATH = "../openssl_files";
const std::string SERVER_CERT = "server.crt";
const std::string SERVER_KEY = "server.key";
const std::string CLIENT_CERT = "client.crt";
const std::string CLIENT_KEY = "client.key";
const std::string CA_CERT = "ca.crt";

std::string hello() { return "Hello World"; }

#ifdef YLT_ENABLE_SSL
TEST_CASE("testing RPC SSL one-way authentication") {
  coro_rpc_server server(2, 8801);

  ssl_configure ssl_conf;
  ssl_conf.base_path = CERT_PATH;
  ssl_conf.cert_file = SERVER_CERT;
  ssl_conf.key_file = SERVER_KEY;
  ssl_conf.ca_cert_file = "";
  ssl_conf.enable_client_verify = false;

  server.init_ssl(ssl_conf);
  server.register_handler<hello>();

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

  auto ec = syncAwait(client.connect("127.0.0.1", "8801"));
  REQUIRE_MESSAGE(!ec, "connect failed");

  auto result = syncAwait(client.call<hello>());
  REQUIRE_MESSAGE(result, "call failed");
  REQUIRE_MESSAGE(result.value() == "Hello World", "result mismatch");

  server.stop();
  io_context.stop();
  thd.join();
}

TEST_CASE("testing RPC SSL mutual authentication - success") {
  coro_rpc_server server(2, 8802);

  ssl_configure ssl_conf;
  ssl_conf.base_path = CERT_PATH;
  ssl_conf.cert_file = SERVER_CERT;
  ssl_conf.key_file = SERVER_KEY;
  ssl_conf.ca_cert_file = CA_CERT;
  ssl_conf.enable_client_verify = true;

  server.init_ssl(ssl_conf);
  server.register_handler<hello>();

  asio::io_context io_context;
  std::thread thd([&io_context]() {
    asio::io_context::work work(io_context);
    io_context.run();
  });

  auto res = server.async_start();
  REQUIRE_MESSAGE(!res.hasResult(), "server start failed");

  coro_rpc_client client;
  bool init_ok =
      client.init_ssl(CERT_PATH, CA_CERT, CLIENT_CERT, CLIENT_KEY, "127.0.0.1");
  REQUIRE_MESSAGE(init_ok == true, "init ssl fail");

  auto ec = syncAwait(client.connect("127.0.0.1", "8802"));
  REQUIRE_MESSAGE(!ec, "connect failed");

  auto result = syncAwait(client.call<hello>());
  REQUIRE_MESSAGE(result, "call failed");
  REQUIRE_MESSAGE(result.value() == "Hello World", "result mismatch");

  server.stop();
  io_context.stop();
  thd.join();
}

TEST_CASE("testing RPC SSL mutual authentication - client without cert") {
  coro_rpc_server server(2, 8803);

  ssl_configure ssl_conf;
  ssl_conf.base_path = CERT_PATH;
  ssl_conf.cert_file = SERVER_CERT;
  ssl_conf.key_file = SERVER_KEY;
  ssl_conf.ca_cert_file = CA_CERT;
  ssl_conf.enable_client_verify = true;

  server.init_ssl(ssl_conf);
  server.register_handler<hello>();

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

  auto ec = syncAwait(client.connect("127.0.0.1", "8803"));
  REQUIRE_MESSAGE(ec, "connect should fail without client cert");
  REQUIRE_MESSAGE(ec == coro_rpc::errc::not_connected,
                  "error should be not_connected");

  server.stop();
  io_context.stop();
  thd.join();
}

TEST_CASE("testing RPC SSL mutual authentication - client with invalid cert") {
  coro_rpc_server server(2, 8804);

  ssl_configure ssl_conf;
  ssl_conf.base_path = CERT_PATH;
  ssl_conf.cert_file = SERVER_CERT;
  ssl_conf.key_file = SERVER_KEY;
  ssl_conf.ca_cert_file = CA_CERT;
  ssl_conf.enable_client_verify = true;

  server.init_ssl(ssl_conf);
  server.register_handler<hello>();

  asio::io_context io_context;
  std::thread thd([&io_context]() {
    asio::io_context::work work(io_context);
    io_context.run();
  });

  auto res = server.async_start();
  REQUIRE_MESSAGE(!res.hasResult(), "server start failed");

  coro_rpc_client client;
  bool init_ok =
      client.init_ssl(CERT_PATH, CA_CERT, "fake.crt", "fake.key", "127.0.0.1");
  REQUIRE_MESSAGE(init_ok == true, "init ssl should succeed");

  auto ec = syncAwait(client.connect("127.0.0.1", "8804"));
  REQUIRE_MESSAGE(ec, "connect should fail with invalid client cert");

  server.stop();
  io_context.stop();
  thd.join();
}
#endif
