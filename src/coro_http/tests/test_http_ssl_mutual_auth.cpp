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

#include <chrono>
#include <iostream>
#include <thread>

#include "async_simple/coro/Lazy.h"
#include "doctest.h"
#include "ylt/coro_http/coro_http_client.hpp"
#include "ylt/coro_http/coro_http_server.hpp"

using namespace coro_http;

using namespace std::chrono_literals;

const std::string CERT_PATH = "../openssl_files/";
const std::string SERVER_CERT = "server.crt";
const std::string SERVER_KEY = "server.key";
const std::string CLIENT_CERT = "client.crt";
const std::string CLIENT_KEY = "client.key";
const std::string CA_CERT = "ca.crt";

#ifdef YLT_ENABLE_SSL
TEST_CASE("testing HTTP SSL one-way authentication") {
  coro_http_server server(1, 8901);

  server.init_ssl((CERT_PATH + SERVER_CERT),
                  (CERT_PATH + SERVER_KEY), "", "", false);

  server.set_http_handler<GET>(
      "/", [](coro_http_request& req, coro_http_response& resp) {
        resp.set_status_and_content(status_type::ok, "Hello World");
      });

  std::thread thd([&server]() {
    server.sync_start();
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  coro_http_client client;
  bool init_ok =
      client.init_ssl(asio::ssl::verify_peer, CERT_PATH, CA_CERT, "127.0.0.1");
  REQUIRE_MESSAGE(init_ok == true, "client init_ssl failed");

  auto result = client.get("http://127.0.0.1:8901/");
  REQUIRE_MESSAGE(result.status == 200, "GET request failed");
  REQUIRE_MESSAGE(result.resp_body == "Hello World", "response body mismatch");

  server.stop();
  thd.join();
}

TEST_CASE("testing HTTP SSL mutual authentication - success") {
  coro_http_server server(1, 8902);

  server.init_ssl((CERT_PATH + SERVER_CERT), (CERT_PATH + SERVER_KEY), "",
                  (CERT_PATH + CA_CERT), true);

  server.set_http_handler<GET>(
      "/", [](coro_http_request& req, coro_http_response& resp) {
        resp.set_status_and_content(status_type::ok, "Hello Mutual Auth");
      });

  std::thread thd([&server]() {
    server.sync_start();
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  coro_http_client client;
  bool init_ok = client.init_ssl(asio::ssl::verify_peer, CERT_PATH, CA_CERT,
                            CLIENT_CERT, CLIENT_KEY, "127.0.0.1");
  REQUIRE_MESSAGE(init_ok == true, "client init_ssl failed");

  auto result = client.get("http://127.0.0.1:8902/");
  REQUIRE_MESSAGE(result.status == 200, "GET request failed");
  REQUIRE_MESSAGE(result.resp_body == "Hello Mutual Auth",
                  "response body mismatch");

  server.stop();
  thd.join();
}

TEST_CASE("testing HTTP SSL mutual authentication - client without cert") {
  coro_http_server server(1, 8903);

  server.init_ssl((CERT_PATH + SERVER_CERT), (CERT_PATH + SERVER_KEY), "",
                  (CERT_PATH + CA_CERT), true);

  server.set_http_handler<GET>(
      "/", [](coro_http_request& req, coro_http_response& resp) {
        resp.set_status_and_content(status_type::ok, "Should not reach here");
      });

  std::thread thd([&server]() {
    server.sync_start();
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  coro_http_client client;
  bool init_ok =
      client.init_ssl(asio::ssl::verify_peer, CERT_PATH, CA_CERT, "127.0.0.1");
  REQUIRE_MESSAGE(init_ok == true, "client init_ssl failed");

  auto result = client.get("http://127.0.0.1:8903/");
  REQUIRE_MESSAGE(result.status != 200,
                  "request should fail without client cert");

  server.stop();
  thd.join();
}

TEST_CASE("testing HTTP SSL mutual authentication - client with invalid cert") {
  coro_http_server server(1, 8904);

  server.init_ssl((CERT_PATH + SERVER_CERT), (CERT_PATH + SERVER_KEY), "",
                  (CERT_PATH + CA_CERT), true);

  server.set_http_handler<GET>(
      "/", [](coro_http_request& req, coro_http_response& resp) {
        resp.set_status_and_content(status_type::ok, "Should not reach here");
      });

  std::thread thd([&server]() {
    server.sync_start();
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  coro_http_client client;
  bool init_ok =
      client.init_ssl(asio::ssl::verify_peer, CERT_PATH, CA_CERT,
                      "invalid_client.crt", "invalid_client.key", "127.0.0.1");
  REQUIRE_MESSAGE(init_ok == true, "client init_ssl should succeed");

  auto result = client.get("http://127.0.0.1:8904/");
  REQUIRE_MESSAGE(result.status != 200,
                  "request should fail with invalid client cert");

  server.stop();
  thd.join();
}

TEST_CASE("testing HTTP SSL mutual authentication - POST request") {
  coro_http_server server(1, 8905);

  server.init_ssl((CERT_PATH + SERVER_CERT), (CERT_PATH + SERVER_KEY), "",
                  (CERT_PATH + CA_CERT), true);

  server.set_http_handler<POST>(
      "/echo",
      [](coro_http_request& req,
         coro_http_response& resp) -> async_simple::coro::Lazy<void> {
        std::string_view body = req.get_body();
        resp.set_status_and_content(status_type::ok,
                                    "Echo: " + std::string(body));
        co_return;
      });

  std::thread thd([&server]() {
    server.sync_start();
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  coro_http_client client;
  bool init_ok = client.init_ssl(asio::ssl::verify_peer, CERT_PATH, CA_CERT,
                            CLIENT_CERT, CLIENT_KEY, "127.0.0.1");
  REQUIRE_MESSAGE(init_ok == true, "client init_ssl failed");

  auto result = client.post("http://127.0.0.1:8905/echo", "Test Message",
                            req_content_type::text);
  REQUIRE_MESSAGE(result.status == 200, "POST request failed");
  REQUIRE_MESSAGE(result.resp_body == "Echo: Test Message",
                  "response body mismatch");

  server.stop();
  thd.join();
}
#endif
