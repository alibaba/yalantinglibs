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

#include <async_simple/coro/Lazy.h>

#include <iostream>
#include <thread>
#include <ylt/coro_http/coro_http_server.hpp>
#include <ylt/coro_io/coro_io.hpp>

using namespace cinatra;

// Certificate paths - adjust these paths to your certificate files
const std::string CERT_PATH = "./certs/rsacerts/";
const std::string SERVER_CERT =
    "server_sign-subca.crt";  // RSA server certificate with full chain
const std::string SERVER_KEY = "server_sign.key";  // RSA server private key
const std::string CA_CERT =
    "chain-ca.crt";  // CA certificate for client verification

// Start one-way authentication SSL server (server certificate only)
void start_one_way_server() {
  std::cout << "Starting SSL HTTP Server (one-way auth) on port 9001..."
            << std::endl;

  coro_http_server server(1, 9001);

  // Initialize SSL server (one-way authentication)
  server.init_ssl((CERT_PATH + SERVER_CERT),  // Server certificate
                  (CERT_PATH + SERVER_KEY),  // Server private key
                  "",  // No password
                  "",  // No CA cert for one-way
                  false  // Don't verify client
  );

  // Root endpoint
  server.set_http_handler<GET>(
      "/",
      [](coro_http_request& req,
         coro_http_response& resp) -> async_simple::coro::Lazy<void> {
        resp.set_status_and_content(status_type::ok,
                                    "Hello World from SSL server (one-way)");
        co_return;
      });

  // Echo endpoint
  server.set_http_handler<POST>(
      "/echo",
      [](coro_http_request& req,
         coro_http_response& resp) -> async_simple::coro::Lazy<void> {
        std::string_view body = req.get_body();
        resp.set_status_and_content(status_type::ok,
                                    "Echo: " + std::string(body));
        co_return;
      });

  std::cout << "One-way SSL server ready. Test endpoints:" << std::endl;
  std::cout << "  GET  https://localhost:9001/" << std::endl;
  std::cout << "  POST https://localhost:9001/echo" << std::endl;

  auto result = server.sync_start();
  if (result) {
    std::cerr << "Failed to start one-way server: " << result.message()
              << std::endl;
  }
}

// Start mutual authentication SSL server (requires client certificate)
void start_mutual_auth_server() {
  std::cout << "Starting SSL HTTP Server (mutual auth) on port 9002..."
            << std::endl;

  coro_http_server server(1, 9002);

  // Initialize SSL server (mutual authentication)
  server.init_ssl(
      (CERT_PATH + SERVER_CERT),  // Server certificate
      (CERT_PATH + SERVER_KEY),  // Server private key
      "",  // No password
      (CERT_PATH + CA_CERT),  // CA certificate for verifying clients
      true  // Verify client certificate (mandatory)
  );

  // Root endpoint
  server.set_http_handler<GET>(
      "/",
      [](coro_http_request& req,
         coro_http_response& resp) -> async_simple::coro::Lazy<void> {
        resp.set_status_and_content(
            status_type::ok, "Hello World from SSL server (mutual auth)");
        co_return;
      });

  // Echo endpoint
  server.set_http_handler<POST>(
      "/echo",
      [](coro_http_request& req,
         coro_http_response& resp) -> async_simple::coro::Lazy<void> {
        std::string_view body = req.get_body();
        resp.set_status_and_content(status_type::ok,
                                    "Echo: " + std::string(body));
        co_return;
      });

  std::cout << "Mutual auth SSL server ready. Test endpoints:" << std::endl;
  std::cout << "  GET  https://localhost:9002/" << std::endl;
  std::cout << "  POST https://localhost:9002/echo" << std::endl;
  std::cout << "Note: Client certificate is REQUIRED for this server"
            << std::endl;

  auto result = server.sync_start();
  if (result) {
    std::cerr << "Failed to start mutual auth server: " << result.message()
              << std::endl;
  }
}

int main() {
  std::cout << "SSL HTTP Server Example - Starting two servers:" << std::endl;
  std::cout << "1. One-way authentication (9001) - Server certificate only"
            << std::endl;
  std::cout << "2. Mutual authentication (9002) - Requires client certificate"
            << std::endl;
  std::cout << "\nMake sure to create the following certificate files in "
            << CERT_PATH << ":" << std::endl;
  std::cout << "  - server.crt: Server certificate" << std::endl;
  std::cout << "  - server.key: Server private key" << std::endl;
  std::cout << "  - ca.crt: CA certificate (for mutual authentication)"
            << std::endl;
  std::cout << "\nYou can generate test certificates using OpenSSL:"
            << std::endl;
  std::cout << "  # Generate CA certificate" << std::endl;
  std::cout << "  openssl genrsa -out ca.key 2048" << std::endl;
  std::cout << "  openssl req -new -x509 -days 365 -key ca.key -out ca.crt"
            << std::endl;
  std::cout << "\n  # Generate server certificate" << std::endl;
  std::cout << "  openssl genrsa -out server.key 2048" << std::endl;
  std::cout << "  openssl req -new -key server.key -out server.csr"
            << std::endl;
  std::cout << "  openssl x509 -req -days 365 -in server.csr -CA ca.crt -CAkey "
               "ca.key -CAcreateserial -out server.crt"
            << std::endl;
  std::cout << "\n  # Generate client certificate (for mutual authentication)"
            << std::endl;
  std::cout << "  openssl genrsa -out client.key 2048" << std::endl;
  std::cout << "  openssl req -new -key client.key -out client.csr"
            << std::endl;
  std::cout << "  openssl x509 -req -days 365 -in client.csr -CA ca.crt -CAkey "
               "ca.key -CAcreateserial -out client.crt"
            << std::endl;

  // Start servers in separate threads
  std::thread one_way_thread(start_one_way_server);
  std::thread mutual_auth_thread(start_mutual_auth_server);

  // Wait for all servers
  one_way_thread.join();
  mutual_auth_thread.join();

  return 0;
}
