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

#include <iostream>
#include <string>
#include <ylt/coro_rpc/coro_rpc_client.hpp>
#include "async_simple/coro/Lazy.h"
#include "async_simple/coro/SyncAwait.h"

using namespace coro_rpc;
using namespace async_simple::coro;
using namespace std::string_literals;

// Certificate paths - adjust to your environment
const std::string CERT_PATH = "E:/vs2022workspace/yalantinglibs/src/certs/sm2/";
const std::string CLIENT_SIGN_CERT = CERT_PATH + "client_sign.crt";  // SM2 client signing certificate
const std::string CLIENT_SIGN_KEY = CERT_PATH + "client_sign.key";   // SM2 client signing private key
const std::string CLIENT_ENC_CERT = CERT_PATH + "client_enc.crt";    // SM2 client encryption certificate
const std::string CLIENT_ENC_KEY = CERT_PATH + "client_enc.key";     // SM2 client encryption private key
const std::string CA_CERT = CERT_PATH + "chain-ca.crt";              // CA certificate

// RPC function declarations (should match server)
std::string echo(std::string_view data);

// Test one-way authentication NTLS client (server certificate only)
Lazy<void> test_one_way_auth() {
  std::cout << "\n=== One-way Authentication NTLS Client (8801) ===" << std::endl;
  
  coro_rpc_client client;

  // Initialize NTLS client for one-way authentication
  // In one-way mode, we only verify server's certificate
  bool ok = client.init_ntls(CERT_PATH,           // Certificate base path
                            "",                   // No client signing certificate needed
                            "",                   // No client signing key needed
                            "",                   // No client encryption certificate needed
                            "",                   // No client encryption key needed
                            CA_CERT,              // CA certificate
                            "localhost",          // Server domain
                            false);               // Don't verify client certificate

  if (!ok) {
    std::cout << "Failed to initialize one-way NTLS client" << std::endl;
    co_return;
  }

  std::cout << "One-way NTLS client initialized successfully" << std::endl;

  // Connect to one-way NTLS server
  auto ec = co_await client.connect("127.0.0.1", "8801");
  if (ec) {
    std::cout << "Failed to connect to one-way server: " << ec.message() << std::endl;
    co_return;
  }

  std::cout << "Connected to one-way NTLS server successfully!" << std::endl;

  try {
    // Test echo function
    auto result = co_await client.call<echo>("Test message to one-way server");
    if (result) {
      std::cout << "Echo result from one-way server: " << result.value() << std::endl;
    } else {
      std::cout << "Echo failed: " << result.error().msg << std::endl;
    }

  } catch (const std::exception& e) {
    std::cout << "RPC call error (one-way): " << e.what() << std::endl;
  }
}

// Test mutual authentication NTLS client (client certificate required)
Lazy<void> test_mutual_auth() {
  std::cout << "\n=== Mutual Authentication NTLS Client (8802) ===" << std::endl;
  
  coro_rpc_client client;

  // Initialize NTLS client for mutual authentication
  // In mutual mode, both client and server certificates are verified

  bool ok = client.init_ntls(
      CERT_PATH,         // Certificate base path
      CLIENT_SIGN_CERT,  // client signing certificate needed
      CLIENT_SIGN_KEY,   // client signing key needed
      CLIENT_ENC_CERT,   // client encryption certificate needed
      CLIENT_ENC_KEY,    // client encryption key needed
      CA_CERT,           // CA certificate
      "localhost",       // Server domain
      true);             // Don't verify client certificate

  if (!ok) {
    std::cout << "Failed to initialize mutual auth NTLS client" << std::endl;
    co_return;
  }

  std::cout << "Mutual auth NTLS client initialized successfully" << std::endl;

  // Connect to mutual auth NTLS server
  auto ec = co_await client.connect("127.0.0.1", "8802");
  if (ec) {
    std::cout << "Failed to connect to mutual auth server: " << ec.message() << std::endl;
    co_return;
  }

  std::cout << "Connected to mutual auth NTLS server successfully!" << std::endl;

  try {
    // Test echo function
    auto result = co_await client.call<echo>("Test message to mutual auth server");
    if (result) {
      std::cout << "Echo result from mutual auth server: " << result.value() << std::endl;
    } else {
      std::cout << "Echo failed: " << result.error().msg << std::endl;
    }

  } catch (const std::exception& e) {
    std::cout << "RPC call error (mutual auth): " << e.what() << std::endl;
  }
}

int main() {
  try {
    std::cout << "NTLS RPC Client Example - Testing two servers:" << std::endl;
    std::cout << "1. One-way authentication (8801) - Server certificate only" << std::endl;
    std::cout << "2. Mutual authentication (8802) - Requires client certificate" << std::endl;

    // Test one-way authentication client
    syncAwait(test_one_way_auth());

    // Test mutual authentication client
    syncAwait(test_mutual_auth());

    std::cout << "\nNTLS RPC Client tests completed." << std::endl;

  } catch (const std::exception& e) {
    std::cout << "Client error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}