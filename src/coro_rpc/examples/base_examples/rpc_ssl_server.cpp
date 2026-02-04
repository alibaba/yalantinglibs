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
#include <string>
#include <thread>
#include <ylt/coro_rpc/coro_rpc_server.hpp>

using namespace coro_rpc;
using namespace std::string_literals;

// Certificate paths - adjust these paths to your certificate files
const std::string CERT_PATH = "./certs/rsacerts";
const std::string SERVER_CERT =
    "server_sign-subca.crt";  // RSA server certificate with full chain
const std::string SERVER_KEY = "server_sign.key";  // RSA server private key
const std::string CA_CERT =
    "chain-ca.crt";  // CA certificate for client verification

// Simple RPC service function
std::string echo(std::string_view data) {
  std::cout << "Server received: " << data << std::endl;
  return "Hello World from SSL server";
}

// Start one-way authentication server (server certificate only)
void start_one_way_server() {
  try {
    coro_rpc_server server(std::thread::hardware_concurrency(), 9001);

    // Configure SSL (one-way authentication)
    ssl_configure ssl_conf;
    ssl_conf.base_path = CERT_PATH;
    ssl_conf.cert_file = SERVER_CERT;
    ssl_conf.key_file = SERVER_KEY;
    ssl_conf.ca_cert_file = "";  // No CA cert needed for one-way
    ssl_conf.enable_client_verify = false;  // Disable client verification

    server.init_ssl(ssl_conf);
    server.register_handler<echo>();

    std::cout << "SSL RPC Server (one-way auth) starting on port 9001..."
              << std::endl;
    std::cout << "Certificate path: " << CERT_PATH << std::endl;

    auto result = server.start();
    if (result) {
      std::cout << "One-way auth server started successfully!" << std::endl;
    }
    else {
      std::cout << "Failed to start one-way auth server: " << result.message()
                << std::endl;
    }

  } catch (const std::exception& e) {
    std::cout << "One-way auth server error: " << e.what() << std::endl;
  }
}

// Start mutual authentication server (client certificate required)
void start_mutual_auth_server() {
  try {
    coro_rpc_server server(std::thread::hardware_concurrency(), 9002);

    // Configure SSL (mutual authentication)
    ssl_configure ssl_conf;
    ssl_conf.base_path = CERT_PATH;
    ssl_conf.cert_file = SERVER_CERT;
    ssl_conf.key_file = SERVER_KEY;
    ssl_conf.ca_cert_file = CA_CERT;  // CA certificate for verifying clients
    ssl_conf.enable_client_verify =
        true;  // Enable client certificate verification

    server.init_ssl(ssl_conf);
    server.register_handler<echo>();

    std::cout << "SSL RPC Server (mutual auth) starting on port 9002..."
              << std::endl;
    std::cout << "Certificate path: " << CERT_PATH << std::endl;
    std::cout << "Client certificate verification: ENABLED (mandatory)"
              << std::endl;

    auto result = server.start();
    if (result) {
      std::cout << "Mutual auth server started successfully!" << std::endl;
    }
    else {
      std::cout << "Failed to start mutual auth server: " << result.message()
                << std::endl;
    }

  } catch (const std::exception& e) {
    std::cout << "Mutual auth server error: " << e.what() << std::endl;
  }
}

int main() {
  std::cout << "SSL RPC Server Example - Starting two servers:" << std::endl;
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
