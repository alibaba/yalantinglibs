/*
 * Copyright (c) 2022, Alibaba Group Holding Limited;
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
#include <coro_rpc/coro_rpc_client.hpp>
#include <coro_rpc/coro_rpc_server.hpp>
#include <coro_rpc/rpc_connection.hpp>

#include "doctest.h"
using namespace coro_rpc;
auto server = std::make_unique<coro_rpc::coro_rpc_server>(
    std::thread::hardware_concurrency(), 8808);

inline std::string test_func(int64_t v1, double v2, std::string v3,
                             const std::array<std::string, 3>& v4) {
  return std::to_string(v1) + std::to_string(v2) + v3 + v4[0] + v4[1] + v4[2];
}
TEST_CASE("test varadic param") {
  using namespace coro_rpc;
  using namespace std;

  bool started = true;
  std::thread thrd([&started] {
    coro_rpc::register_handler<test_func>();
    try {
      auto ec = server->start();
      REQUIRE(ec == std::errc{});
    } catch (const std::exception& e) {
      started = false;
      std::cerr << "test varadic param Exception: " << e.what() << "\n";
    }
  });
  REQUIRE_MESSAGE(server->wait_for_start(3s), "server start timeout");
  coro_rpc_client client;

  syncAwait(client.connect("localhost", std::to_string(server->port())));

  auto ret = syncAwait(client.call<test_func>(114514, 2.0f, "Hello coro_rpc!",
                                              std::array{
                                                  "hello"s,
                                                  "hi"s,
                                                  "what"s,
                                              }));
  easylog::info("begin to stop server");
  server->stop();
  if (thrd.joinable())
    thrd.join();
  easylog::info("finished stop server");
  if (ret) {
    easylog::info("ret value {}", ret.value());
  }
  else {
    easylog::info("ret err msg {}", ret.error().msg);
  }

  CHECK(ret);
  if (ret) {
    CHECK(ret == "1145142.000000Hello coro_rpc!hellohiwhat");
    std::cout << ret.value() << std::endl;
  }
}