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
#include <ylt/coro_http/coro_http_client.hpp>
#include <ylt/coro_io/coro_io.hpp>

using namespace cinatra;

// Certificate paths - adjust to your environment
const std::string CERT_PATH = "E:/vs2022workspace/yalantinglibs/src/certs/sm2/";
const std::string CLIENT_SIGN_CERT = CERT_PATH + "client_sign.crt";  // SM2 client signing certificate
const std::string CLIENT_SIGN_KEY = CERT_PATH + "client_sign.key";   // SM2 client signing private key
const std::string CLIENT_ENC_CERT = CERT_PATH + "client_enc.crt";    // SM2 client encryption certificate
const std::string CLIENT_ENC_KEY = CERT_PATH + "client_enc.key";     // SM2 client encryption private key
const std::string CA_CERT = CERT_PATH + "chain-ca.crt";              // CA certificate

// Test one-way authentication NTLS client (server certificate only)
async_simple::coro::Lazy<void> test_ntls_http_client_one_way() {
  std::cout << "\n=== One-way Authentication NTLS Client (8801) ===" << std::endl;
  
  coro_http_client client{};
  
  // Enable NTLS protocol (simple mode, server authentication only)
  client.set_ntls_schema(true);
  
  std::cout << "NTLS HTTP Client initialized (one-way auth mode)" << std::endl;
  
  // Send GET request to root endpoint
  auto response = co_await client.async_get("https://localhost:8801/");
  if (response.net_err) {
    std::cout << "GET / request failed: " << response.net_err.message() << std::endl;
  } else {
    std::cout << "GET / Response status: " << response.status << std::endl;
    std::cout << "GET / Response body: " << response.resp_body << std::endl;
  }
  
  // Send POST request to echo endpoint
  auto post_response = co_await client.async_post("https://localhost:8801/echo",
                                                  "Test message",
                                                  req_content_type::text);
  if (post_response.net_err) {
    std::cout << "POST /echo request failed: " << post_response.net_err.message() << std::endl;
  } else {
    std::cout << "POST /echo Response status: " << post_response.status << std::endl;
    std::cout << "POST /echo Response body: " << post_response.resp_body << std::endl;
  }
}

// Test mutual authentication NTLS client (client certificate required)
async_simple::coro::Lazy<void> test_ntls_http_client_mutual_auth() {
  std::cout << "\n=== Mutual Authentication NTLS Client (8802) ===" << std::endl;
  
  coro_http_client client{};
  
  // Initialize NTLS client with mutual authentication
  bool init_ok = client.init_ntls_client(
    CLIENT_SIGN_CERT,  // Client SM2 signing certificate
    CLIENT_SIGN_KEY,   // Client SM2 signing private key
    CLIENT_ENC_CERT,   // Client SM2 encryption certificate
    CLIENT_ENC_KEY,    // Client SM2 encryption private key
    CA_CERT,           // CA certificate
    asio::ssl::verify_peer  // Verify server certificate
  );
  
  if (!init_ok) {
    std::cout << "Failed to initialize NTLS client with mutual authentication" << std::endl;
    co_return;
  }
  
  std::cout << "NTLS HTTP Client initialized (mutual authentication mode)" << std::endl;
  
  // Send GET request to root endpoint (requires client certificate)
  auto response = co_await client.async_get("https://localhost:8802/");
  if (response.net_err) {
    std::cout << "GET / request failed: " << response.net_err.message() << std::endl;
  } else {
    std::cout << "GET / Response status: " << response.status << std::endl;
    std::cout << "GET / Response body: " << response.resp_body << std::endl;
  }
  
  // Send POST request to echo endpoint (requires client certificate)
  auto post_response = co_await client.async_post("https://localhost:8802/echo",
                                                  "Test message with client certificate",
                                                  req_content_type::text);
  if (post_response.net_err) {
    std::cout << "POST /echo request failed: " << post_response.net_err.message() << std::endl;
  } else {
    std::cout << "POST /echo Response status: " << post_response.status << std::endl;
    std::cout << "POST /echo Response body: " << post_response.resp_body << std::endl;
  }
}

int main() {
  std::cout << "Starting NTLS HTTP Client Examples..." << std::endl;
  std::cout << "Testing NTLS/TLCP protocol with SM2/SM3/SM4 algorithms" << std::endl;
  
  try {
    // Test one-way authentication NTLS client (port 8801)
    //async_simple::coro::syncAwait(test_ntls_http_client_one_way());
    
    // Test mutual authentication NTLS client (port 8802)
    async_simple::coro::syncAwait(test_ntls_http_client_mutual_auth());
    
  } catch (const std::exception& e) {
    std::cout << "Exception: " << e.what() << std::endl;
    return 1;
  }
  
  std::cout << "\nNTLS HTTP Client Examples completed." << std::endl;
  return 0;
}