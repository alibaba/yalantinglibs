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

#include <iostream>
#include <thread>
#include <ylt/coro_http/coro_http_server.hpp>
#include <ylt/coro_io/coro_io.hpp>

using namespace cinatra;

// Certificate paths - adjust to your environment
const std::string CERT_PATH = "E:/vs2022workspace/yalantinglibs/src/certs/sm2/";
const std::string SERVER_SIGN_CERT = CERT_PATH + "server_sign.crt";  // SM2 signing certificate
const std::string SERVER_SIGN_KEY = CERT_PATH + "server_sign.key";   // SM2 signing private key
const std::string SERVER_ENC_CERT = CERT_PATH + "server_enc.crt";    // SM2 encryption certificate
const std::string SERVER_ENC_KEY = CERT_PATH + "server_enc.key";     // SM2 encryption private key
const std::string CA_CERT = CERT_PATH + "chain-ca.crt";              // CA certificate

// Start one-way authentication NTLS server (server certificate only)
void start_one_way_server() {
  std::cout << "Starting NTLS HTTP Server (one-way auth) on port 8801..." << std::endl;
  
  coro_http_server server(1, 8801);
  
  // Initialize NTLS server with dual certificates (SM2 sign/encrypt)
  // One-way authentication mode (no client certificate verification)
  server.init_ntls(SERVER_SIGN_CERT,
                   SERVER_SIGN_KEY,
                   SERVER_ENC_CERT,
                   SERVER_ENC_KEY,
                   CA_CERT,
                   false);  // false = don't verify client certificate
  
  // Set NTLS cipher suites
  server.set_ntls_cipher_suites("ECC-SM2-SM4-GCM-SM3:ECC-SM2-SM4-CBC-SM3");
  
  // Root endpoint
  server.set_http_handler<GET>("/", [](coro_http_request& req, coro_http_response& resp) -> async_simple::coro::Lazy<void> {
    resp.set_status_and_content(status_type::ok, "Hello World");
    co_return;
  });
  
  // Echo endpoint
  server.set_http_handler<POST>("/echo", [](coro_http_request& req, coro_http_response& resp) -> async_simple::coro::Lazy<void> {
    resp.set_status_and_content(status_type::ok, "Hello World");
    co_return;
  });
  
  std::cout << "One-way NTLS server ready. Test endpoints:" << std::endl;
  std::cout << "  GET  https://localhost:8801/" << std::endl;
  std::cout << "  POST https://localhost:8801/echo" << std::endl;
  
  auto result = server.sync_start();
  if (result) {
    std::cerr << "Failed to start one-way server: " << result.message() << std::endl;
  }
}

// Start mutual authentication NTLS server (requires client certificate)
void start_mutual_auth_server() {
  std::cout << "Starting NTLS HTTP Server (mutual auth) on port 8802..." << std::endl;
  
  coro_http_server server(1, 8802);
  
  // Initialize NTLS server with dual certificates (SM2 sign/encrypt)
  // Mutual authentication mode (client certificate verification enabled)
  server.init_ntls(SERVER_SIGN_CERT,
                   SERVER_SIGN_KEY,
                   SERVER_ENC_CERT,
                   SERVER_ENC_KEY,
                   CA_CERT,
                   true);  // true = verify client certificate
  
  // Set NTLS cipher suites
  server.set_ntls_cipher_suites("ECC-SM2-SM4-GCM-SM3:ECC-SM2-SM4-CBC-SM3");
  
  // Root endpoint
  server.set_http_handler<GET>("/", [](coro_http_request& req, coro_http_response& resp) -> async_simple::coro::Lazy<void> {
    resp.set_status_and_content(status_type::ok, "Hello World");
    co_return;
  });
  
  // Echo endpoint
  server.set_http_handler<POST>("/echo", [](coro_http_request& req, coro_http_response& resp) -> async_simple::coro::Lazy<void> {
    resp.set_status_and_content(status_type::ok, "Hello World");
    co_return;
  });
  
  std::cout << "Mutual auth NTLS server ready. Test endpoints:" << std::endl;
  std::cout << "  GET  https://localhost:8802/ (requires client certificate)" << std::endl;
  std::cout << "  POST https://localhost:8802/echo (requires client certificate)" << std::endl;
  
  auto result = server.sync_start();
  if (result) {
    std::cerr << "Failed to start mutual auth server: " << result.message() << std::endl;
  }
}

int main() {
  std::cout << "NTLS HTTP Server Example - Starting two servers:" << std::endl;
  std::cout << "1. One-way authentication (8801) - Server certificate only" << std::endl;
  std::cout << "2. Mutual authentication (8802) - Requires client certificate" << std::endl;
  
  // Start both servers in separate threads
  std::thread one_way_thread(start_one_way_server);
  std::thread mutual_auth_thread(start_mutual_auth_server);
  
  // Wait for both servers
  one_way_thread.join();
  mutual_auth_thread.join();
  
  return 0;
}