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

// Certificate paths - adjust to your environment
const std::string CERT_PATH = "E:/vs2022workspace/yalantinglibs/src/certs/sm2/";
const std::string SERVER_SIGN_CERT = CERT_PATH + "server_sign.crt";  // SM2 server signing certificate
const std::string SERVER_SIGN_KEY = CERT_PATH + "server_sign.key";   // SM2 server signing private key
const std::string SERVER_ENC_CERT = CERT_PATH + "server_enc.crt";    // SM2 server encryption certificate
const std::string SERVER_ENC_KEY = CERT_PATH + "server_enc.key";     // SM2 server encryption private key
const std::string CA_CERT = CERT_PATH + "chain-ca.crt";              // CA certificate

// Simple RPC service function
std::string echo(std::string_view data) {
  std::cout << "Server received: " << data << std::endl;
  return "Hello World";
}

// Start one-way authentication server (server certificate only)
void start_one_way_server() {
  try {
    // Create RPC server for one-way authentication
    coro_rpc_server server(std::thread::hardware_concurrency(), 8801);

    // Configure NTLS with Tongsuo (one-way authentication)
    ssl_ntls_configure ntls_conf;
    ntls_conf.base_path = "";                         // Using full paths instead
    ntls_conf.sign_cert_file = SERVER_SIGN_CERT;      // SM2 signing certificate
    ntls_conf.sign_key_file = SERVER_SIGN_KEY;        // SM2 signing private key
    ntls_conf.enc_cert_file = SERVER_ENC_CERT;        // SM2 encryption certificate
    ntls_conf.enc_key_file = SERVER_ENC_KEY;          // SM2 encryption private key
    ntls_conf.ca_cert_file = CA_CERT;                 // CA certificate
    ntls_conf.enable_client_verify = false;           // Disable client certificate verification
    ntls_conf.enable_ntls = true;                     // Enable NTLS mode

    // Initialize NTLS
    server.init_ntls(ntls_conf);

    // Register RPC service function
    server.register_handler<echo>();

    std::cout << "NTLS RPC Server (one-way auth) starting on port 8801..." << std::endl;
    std::cout << "Using Tongsuo for NTLS support" << std::endl;
    std::cout << "Certificate path: " << CERT_PATH << std::endl;

    // Start server (blocking)
    auto result = server.start();
    if (result) {
      std::cout << "One-way auth server started successfully!" << std::endl;
    } else {
      std::cout << "Failed to start one-way auth server: " << result.message() << std::endl;
    }

  } catch (const std::exception& e) {
    std::cout << "One-way auth server error: " << e.what() << std::endl;
  }
}

// Start mutual authentication server (client certificate required)
void start_mutual_auth_server() {
  try {
    // Create RPC server for mutual authentication
    coro_rpc_server server(std::thread::hardware_concurrency(), 8802);

    // Configure NTLS with Tongsuo (mutual authentication)
    ssl_ntls_configure ntls_conf;
    ntls_conf.base_path = "";                         // Using full paths instead
    ntls_conf.sign_cert_file = SERVER_SIGN_CERT;      // SM2 signing certificate
    ntls_conf.sign_key_file = SERVER_SIGN_KEY;        // SM2 signing private key
    ntls_conf.enc_cert_file = SERVER_ENC_CERT;        // SM2 encryption certificate
    ntls_conf.enc_key_file = SERVER_ENC_KEY;          // SM2 encryption private key
    ntls_conf.ca_cert_file = CA_CERT;                 // CA certificate
    ntls_conf.enable_client_verify = true;            // Enable client certificate verification
    ntls_conf.enable_ntls = true;                     // Enable NTLS mode

    // Initialize NTLS
    server.init_ntls(ntls_conf);

    // Register RPC service function
    server.register_handler<echo>();

    std::cout << "NTLS RPC Server (mutual auth) starting on port 8802..." << std::endl;
    std::cout << "Using Tongsuo for NTLS support" << std::endl;
    std::cout << "Certificate path: " << CERT_PATH << std::endl;

    // Start server (blocking)
    auto result = server.start();
    if (result) {
      std::cout << "Mutual auth server started successfully!" << std::endl;
    } else {
      std::cout << "Failed to start mutual auth server: " << result.message() << std::endl;
    }

  } catch (const std::exception& e) {
    std::cout << "Mutual auth server error: " << e.what() << std::endl;
  }
}

int main() {
  std::cout << "NTLS RPC Server Example - Starting two servers:" << std::endl;
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