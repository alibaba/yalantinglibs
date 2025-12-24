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

#include <memory>
#include <utility>
#include <ylt/coro_rpc/coro_rpc_client.hpp>
#include <ylt/coro_rpc/coro_rpc_server.hpp>

#include "doctest.h"
#include "rpc_api.hpp"
#include "ylt/coro_io/server_acceptor.hpp"
#include "ylt/coro_rpc/impl/default_config/coro_rpc_config.hpp"

using namespace coro_rpc;

TEST_CASE("test server acceptor") {
  SUBCASE("test multi server acceptor") {
    std::vector<std::unique_ptr<coro_io::server_acceptor_base>> acceptors;
    acceptors.emplace_back(
        std::make_unique<coro_io::tcp_server_acceptor>("0.0.0.0", 8824));
    acceptors.emplace_back(
        std::make_unique<coro_io::tcp_server_acceptor>("localhost", 8825));
    coro_rpc_server server(
        coro_rpc::config_t{.acceptors = std::move(acceptors)});
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
}