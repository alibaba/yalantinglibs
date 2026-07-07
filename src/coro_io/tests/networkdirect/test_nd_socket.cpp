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
#include <array>
#include <cstdint>
#include <exception>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>

#include "asio/buffer.hpp"
#include "asio/io_context.hpp"
#include "asio/ip/tcp.hpp"
#include "async_simple/coro/Collect.h"
#include "async_simple/coro/Lazy.h"
#include "async_simple/coro/SyncAwait.h"
#include "doctest.h"
#include "ylt/coro_io/io_context_pool.hpp"
#include "ylt/coro_io/networkdirect/nd.hpp"

using namespace coro_io;

namespace {

constexpr std::size_t kBufSize = 64 * 1024;

nd_socket_t::config_t make_config(nd_device_ptr device) {
  return nd_socket_t::config_t{.buffer_size = kBufSize, .device = device};
}

// Server: accept one connection, echo `echo_count` messages back to the client.
async_simple::coro::Lazy<std::error_code> echo_server(
    nd_listener<tcp>& listener, ExecutorWrapper<>* ex, nd_device_ptr device,
    int echo_count) {
  nd_socket_t sock(ex, make_config(device));
  auto ec = co_await async_accept(listener, sock);
  if (ec) {
    co_return ec;
  }
  std::string buf;
  buf.resize(kBufSize);
  for (int i = 0; i < echo_count; ++i) {
    auto [rec, n] =
        co_await async_read_some(sock, asio::buffer(buf.data(), buf.size()));
    if (rec || n == 0) {
      co_return rec;
    }
    auto [sec, sn] = co_await async_write(sock, asio::buffer(buf.data(), n));
    if (sec) {
      co_return sec;
    }
    CHECK(sn == n);
  }
  co_return std::error_code{};
}

// Client: connect, then send/recv `echo_count` messages and verify the echo.
async_simple::coro::Lazy<std::error_code> echo_client(
    ExecutorWrapper<>* ex, nd_device_ptr device, std::string host,
    std::uint16_t port, int echo_count) {
  nd_socket_t sock(ex, make_config(device));
  auto ec = co_await async_connect(sock, host, std::to_string(port));
  if (ec) {
    co_return ec;
  }
  for (int i = 0; i < echo_count; ++i) {
    std::string msg = "Hello NetworkDirect #" + std::to_string(i);
    auto [sec, sn] = co_await async_write(sock, asio::buffer(msg));
    if (sec) {
      co_return sec;
    }
    CHECK(sn == msg.size());

    std::string in;
    in.resize(msg.size());
    auto [rec, rn] = co_await async_read(sock, asio::buffer(in.data(), in.size()));
    if (rec) {
      co_return rec;
    }
    CHECK(rn == msg.size());
    CHECK(in == msg);
  }
  co_return std::error_code{};
}

nd_device_ptr try_get_device() {
  try {
    return nd_device_manager_t::instance().get_first_available_device({});
  } catch (std::exception const& e) {
    MESSAGE("no ND device available, skipping data-plane test: " << e.what());
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

TEST_CASE("nd_socket connect + echo (loopback)") {
  auto device = try_get_device();
  if (!device) {
    return;  // runtime-skip when no ND adapter is present
  }

  std::string host;
  try {
    host = device->get_v4_address().to_string();
  } catch (std::exception const& e) {
    MESSAGE("ND device has no IPv4 address, skipping: " << e.what());
    return;
  }

  asio::io_context io_ctx;
  asio::error_code uec;
  use_device(io_ctx, device, {}, uec);
  if (uec) {
    MESSAGE("use_device failed, skipping: " << uec.message());
    return;
  }

  ExecutorWrapper<> ex(io_ctx.get_executor());
  auto work = asio::make_work_guard(io_ctx);
  std::thread io_thread([&] {
    io_ctx.run();
  });

  auto port = pick_unused_tcp_port();
  REQUIRE_MESSAGE(port != 0, "failed to pick an unused ND socket test port");
  nd_listener<tcp> listener(io_ctx);
  listener.open(tcp::v4());
  listener.bind(port);
  listener.listen();

  constexpr int echo_count = 10;
  auto result = async_simple::coro::syncAwait(async_simple::coro::collectAll(
      echo_server(listener, &ex, device, echo_count),
      echo_client(&ex, device, host, port, echo_count)));

  auto server_ec = std::get<0>(result).value();
  auto client_ec = std::get<1>(result).value();
  CHECK_MESSAGE(!server_ec, "server: " << server_ec.message());
  CHECK_MESSAGE(!client_ec, "client: " << client_ec.message());

  work.reset();
  io_ctx.stop();
  io_thread.join();
}
