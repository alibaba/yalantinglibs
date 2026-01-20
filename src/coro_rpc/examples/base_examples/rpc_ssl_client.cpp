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

#include <iostream>
#include <ylt/coro_rpc/coro_rpc_client.hpp>

using namespace coro_rpc;
using namespace async_simple::coro;

// Certificate paths - adjust these paths to your certificate files
const std::string CERT_PATH = "./certs/rsacerts";
const std::string CA_CERT =
    "chain-ca.crt";  // Root CA certificate to verify server
const std::string CLIENT_CERT =
    "client_sign.crt";  // Client certificate for mutual auth
const std::string CLIENT_KEY =
    "client_sign.key";  // Client private key for mutual auth

// Declare RPC function (must match server side)
std::string echo(std::string_view data);

// Test one-way authentication (server certificate only)
Lazy<void> test_one_way_auth() {
  try {
    coro_rpc_client client;

    // Initialize SSL with CA certificate for server verification
    bool init_result = client.init_ssl(CERT_PATH, CA_CERT, "127.0.0.1");
    if (!init_result) {
      std::cout << "Failed to initialize SSL client" << std::endl;
      co_return;
    }

    // Connect to server
    auto ec = co_await client.connect("127.0.0.1", "9001");
    if (ec) {
      std::cout << "Failed to connect to server: " << ec.message() << std::endl;
      co_return;
    }

    std::cout << "Connected to one-way auth server" << std::endl;

    // Call RPC function
    auto result = co_await client.call<echo>("Hello from one-way client");
    if (result) {
      std::cout << "RPC call successful: " << result.value() << std::endl;
    }
    else {
      std::cout << "RPC call failed: " << result.error().msg << std::endl;
    }

  } catch (const std::exception& e) {
    std::cout << "One-way auth client error: " << e.what() << std::endl;
  }
}

// Test mutual authentication (client certificate required)
Lazy<void> test_mutual_auth() {
  try {
    coro_rpc_client client;

    // Initialize SSL with CA certificate AND client certificate for mutual auth
    bool init_result =
        client.init_ssl(CERT_PATH,  // Base path
                        CA_CERT,  // CA certificate for server verification
                        CLIENT_CERT,  // Client certificate
                        CLIENT_KEY,  // Client private key
                        "127.0.0.1"  // Server hostname
        );

    if (!init_result) {
      std::cout << "Failed to initialize SSL client with mutual authentication"
                << std::endl;
      co_return;
    }

    // Connect to server
    auto ec = co_await client.connect("127.0.0.1", "9002");
    if (ec) {
      std::cout << "Failed to connect to mutual auth server: " << ec.message()
                << std::endl;
      co_return;
    }

    std::cout << "Connected to mutual auth server" << std::endl;

    // Call RPC function
    auto result = co_await client.call<echo>("Hello from mutual auth client");
    if (result) {
      std::cout << "RPC call successful: " << result.value() << std::endl;
    }
    else {
      std::cout << "RPC call failed: " << result.error().msg << std::endl;
    }

  } catch (const std::exception& e) {
    std::cout << "Mutual auth client error: " << e.what() << std::endl;
  }
}

int main() {
  std::cout << "SSL RPC Client Example" << std::endl;
  std::cout << "Certificate path: " << CERT_PATH << std::endl;
  std::cout << "\nMake sure the server is running before starting the client"
            << std::endl;
  std::cout << "\nTesting one-way authentication..." << std::endl;

  // Test one-way authentication
  syncAwait(test_one_way_auth());

  std::cout << "\nTesting mutual authentication..." << std::endl;

  // Test mutual authentication
  syncAwait(test_mutual_auth());

  return 0;
}
