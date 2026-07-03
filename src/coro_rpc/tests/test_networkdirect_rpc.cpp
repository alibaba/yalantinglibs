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
#ifdef YLT_ENABLE_ND

#include <cstdint>
#include <exception>
#include <string>

#include "asio/io_context.hpp"
#include "asio/ip/tcp.hpp"
#include "async_simple/coro/SyncAwait.h"
#include "doctest.h"
#include "rpc_api.hpp"
#include "ylt/coro_io/networkdirect/nd.hpp"
#include "ylt/coro_rpc/coro_rpc_client.hpp"
#include "ylt/coro_rpc/coro_rpc_server.hpp"

namespace {

coro_io::nd_device_ptr try_get_nd_device() {
  try {
    return coro_io::nd_device_manager_t::instance().get_first_available_device(
        {});
  } catch (const std::exception &e) {
    MESSAGE("no ND device available, skipping RPC ND test: " << e.what());
    return nullptr;
  }
}

std::uint16_t pick_unused_tcp_port() {
  asio::io_context ctx;
  asio::ip::tcp::acceptor acceptor(ctx);
  asio::error_code ec;

  acceptor.open(asio::ip::tcp::v4(), ec);
  if (ec) {
    return 0;
  }
  acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true), ec);
  if (ec) {
    return 0;
  }
  acceptor.bind({asio::ip::tcp::v4(), 0}, ec);
  if (ec) {
    return 0;
  }

  auto endpoint = acceptor.local_endpoint(ec);
  if (ec) {
    return 0;
  }
  auto port = endpoint.port();
  acceptor.close(ec);
  return port;
}

}  // namespace

TEST_CASE("coro_rpc over NetworkDirect") {
  auto device = try_get_nd_device();
  if (!device) {
    return;
  }

  std::string host;
  try {
    host = device->get_v4_address().to_string();
  } catch (const std::exception &e) {
    MESSAGE("ND device has no IPv4 address, skipping RPC ND test: "
            << e.what());
    return;
  }
  auto nd_port = pick_unused_tcp_port();
  REQUIRE_MESSAGE(nd_port != 0, "failed to pick an unused ND RPC port");

  coro_io::nd_socket_t::config_t nd_config{
      .buffer_size = 64 * 1024,
      .device = device,
  };

  coro_rpc::config_t server_config;
  server_config.thread_num = 2;
  server_config.port = 0;
  server_config.address = "0.0.0.0";
  server_config.nd_config = nd_config;
  server_config.nd_port = nd_port;
  server_config.nd_address = host;

  coro_rpc::coro_rpc_server server(server_config);
  server.register_handler<hello, large_arg_fun>();
  auto server_future = server.async_start();
  REQUIRE_MESSAGE(!server_future.hasResult(), "server start failed");

  coro_rpc::coro_rpc_client client;
  REQUIRE(client.init_nd(nd_config));
  auto ec = async_simple::coro::syncAwait(
      client.connect(host, std::to_string(nd_port)));
  REQUIRE_MESSAGE(!ec, "connect failed: " << ec.message());

  auto hello_result = async_simple::coro::syncAwait(client.call<hello>());
  REQUIRE(hello_result);
  CHECK(hello_result.value() == "hello");

  std::string payload(128 * 1024, 'n');
  auto large_result = async_simple::coro::syncAwait(
      client.call<large_arg_fun>(payload));
  REQUIRE(large_result);
  CHECK(large_result.value() == payload);

  client.close();
  server.stop();
}

#endif  // YLT_ENABLE_ND
