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

#include <asio/io_context.hpp>
#include <asio/ip/v6_only.hpp>
#include <memory>
#include <utility>
#include <vector>
#include <ylt/coro_rpc/coro_rpc_client.hpp>
#include <ylt/coro_rpc/coro_rpc_server.hpp>

#include "doctest.h"
#include "rpc_api.hpp"
#include "ylt/coro_io/server_acceptor.hpp"
#include "ylt/coro_rpc/impl/default_config/coro_rpc_config.hpp"

using namespace coro_rpc;

namespace {
asio::error_code bind_ipv6_only_probe(uint16_t port) {
  asio::io_context ctx;
  asio::ip::tcp::acceptor probe(ctx);
  asio::error_code ec;
  probe.open(asio::ip::tcp::v6(), ec);
  if (ec) {
    return ec;
  }
  probe.set_option(asio::ip::v6_only(true), ec);
  if (ec) {
    return ec;
  }
  probe.bind({asio::ip::address_v6::any(), port}, ec);
  return ec;
}
}  // namespace

#ifdef YLT_ENABLE_IBV
std::string addr;
void test_rdma_multi_dev_server() {
  auto addr_now =
      coro_rpc::get_context()->get_local_endpoint().address.to_string();
  if (addr.size()) {
    if (coro_io::g_ib_device_manager()->get_dev_list().size() > 1) {
      // CHECK(addr_now != addr);
    }
    else {
      CHECK(addr_now == addr);
    }
  }
  addr = addr_now;
}
#endif

TEST_CASE("test server acceptor") {
  SUBCASE("test ipv6 any acceptor layout") {
    coro_rpc_server server(static_cast<size_t>(1),
                           static_cast<unsigned short>(8826),
                           std::string("::"));
    const auto& acceptors = server.get_acceptors();
#if defined(__linux__)
    REQUIRE(acceptors.size() == 2);
    CHECK(acceptors[0]->address() == "::");
    CHECK(acceptors[1]->address() == "0.0.0.0");
    CHECK(acceptors[0]->port() == 8826);
    CHECK(acceptors[1]->port() == 8826);
#else
    REQUIRE(acceptors.size() == 1);
    CHECK(acceptors[0]->address() == "::");
    CHECK(acceptors[0]->port() == 8826);
#endif
  }

  SUBCASE("test ipv6 any accepts ipv4 and ipv6 rpc clients") {
    coro_rpc_server server(static_cast<size_t>(1),
                           static_cast<unsigned short>(0), std::string("::"));
    server.register_handler<hello>();

    auto res = server.async_start();
    CHECK_MESSAGE(!res.hasResult(), "server start timeout");
    REQUIRE(server.port() > 0);

    const auto port = std::to_string(server.port());

    coro_rpc_client client_v4;
    auto ec = syncAwait(client_v4.connect("127.0.0.1", port));
    CHECK_MESSAGE(!ec, ec.message());
    auto result = syncAwait(client_v4.call<hello>());
    CHECK_MESSAGE(result.has_value(), result.error().msg);

    coro_rpc_client client_v6;
    ec = syncAwait(client_v6.connect("::1", port));
    CHECK_MESSAGE(!ec, ec.message());
    result = syncAwait(client_v6.call<hello>());
    CHECK_MESSAGE(result.has_value(), result.error().msg);

#if defined(__linux__)
    CHECK(server.get_acceptors().size() == 2);
    CHECK(server.get_acceptors()[1]->address() == "0.0.0.0");
    CHECK(server.get_acceptors()[1]->port() == server.port());
#endif

    server.stop();
  }

  SUBCASE("test ipv6 any address string creates dual acceptors") {
    coro_rpc_server server(static_cast<size_t>(1), std::string("[::]:8827"));
    const auto& acceptors = server.get_acceptors();
#if defined(__linux__)
    REQUIRE(acceptors.size() == 2);
    CHECK(acceptors[0]->address() == "::");
    CHECK(acceptors[1]->address() == "0.0.0.0");
    CHECK(acceptors[0]->port() == 8827);
    CHECK(acceptors[1]->port() == 8827);
#else
    REQUIRE(acceptors.size() == 1);
    CHECK(acceptors[0]->address() == "::");
    CHECK(acceptors[0]->port() == 8827);
#endif
  }

  SUBCASE("test ipv6 any address string with port 0 creates dual acceptors") {
    coro_rpc_server server(static_cast<size_t>(1), std::string("[::]:0"));
    server.register_handler<hello>();

    auto res = server.async_start();
    CHECK_MESSAGE(!res.hasResult(), "server start timeout");
    REQUIRE(server.port() > 0);

#if defined(__linux__)
    const auto& acceptors = server.get_acceptors();
    REQUIRE(acceptors.size() == 2);
    CHECK(acceptors[0]->address() == "::");
    CHECK(acceptors[1]->address() == "0.0.0.0");
    CHECK(acceptors[0]->port() == server.port());
    CHECK(acceptors[1]->port() == server.port());
#endif

    server.stop();
  }

#if defined(__linux__)
  SUBCASE("test failed second acceptor closes primary acceptor") {
    asio::io_context ctx;
    asio::ip::tcp::acceptor ipv4_blocker(ctx);
    asio::error_code ec;
    ipv4_blocker.open(asio::ip::tcp::v4(), ec);
    REQUIRE(!ec);
    ipv4_blocker.bind({asio::ip::address_v4::any(), 0}, ec);
    REQUIRE(!ec);
    ipv4_blocker.listen(asio::socket_base::max_listen_connections, ec);
    REQUIRE(!ec);

    auto port = ipv4_blocker.local_endpoint().port();

    coro_rpc_server server(static_cast<size_t>(1), port, std::string("::"));
    auto start_error = server.async_start().get();
    REQUIRE(start_error);

    auto probe_error = bind_ipv6_only_probe(port);
    CHECK_MESSAGE(!probe_error, probe_error.message());
  }
#endif

  SUBCASE("test multi server acceptor") {
    std::vector<std::unique_ptr<coro_io::server_acceptor_base>> acceptors;
    acceptors.emplace_back(
        std::make_unique<coro_io::tcp_server_acceptor>("0.0.0.0", 8824));
    acceptors.emplace_back(
        std::make_unique<coro_io::tcp_server_acceptor>("localhost", 8825));
    coro_rpc_server server(coro_rpc::config_t{}, std::move(acceptors));
    server.register_handler<hello>();

    auto res = server.async_start();
    CHECK_MESSAGE(!res.hasResult(), "server start timeout");

    coro_rpc_client client;
    auto ec = syncAwait(client.connect("127.0.0.1", "8824"));
    CHECK_MESSAGE(!ec, ec.message());

    auto result = syncAwait(client.call<hello>());
    CHECK_MESSAGE(result.has_value(), result.error().msg);

    coro_rpc_client client2;
    ec = syncAwait(client2.connect("localhost", "8825"));
    CHECK_MESSAGE(!ec, ec.message());

    result = syncAwait(client2.call<hello>());
    CHECK_MESSAGE(result.has_value(), result.error().msg);

    server.stop();
  }

#ifdef YLT_ENABLE_IBV
  SUBCASE("test multi rdma device for server") {
    std::vector<std::shared_ptr<coro_io::ib_device_t>> ibv_dev_lists;
    for (auto& dev : coro_io::g_ib_device_manager()->get_dev_list()) {
      ibv_dev_lists.push_back(dev.second);
    }
    coro_rpc_server server(
        coro_rpc::config_t{.port = 8824,
                           .thread_num = 1,
                           .ibv_config = {},
                           .ibv_dev_lists = std::move(ibv_dev_lists)});
    server.register_handler<test_rdma_multi_dev_server>();

    auto res = server.async_start();
    CHECK_MESSAGE(!res.hasResult(), "server start timeout");

    coro_rpc_client client;
    auto ec = syncAwait(client.connect("127.0.0.1", "8824"));
    CHECK_MESSAGE(!ec, ec.message());

    auto result = syncAwait(client.call<test_rdma_multi_dev_server>());
    CHECK_MESSAGE(result.has_value(), result.error().msg);

    coro_rpc_client client2;
    ec = syncAwait(client2.connect("localhost", "8824"));
    CHECK_MESSAGE(!ec, ec.message());

    result = syncAwait(client2.call<test_rdma_multi_dev_server>());
    CHECK_MESSAGE(result.has_value(), result.error().msg);

    server.stop();
  }
#endif
}
