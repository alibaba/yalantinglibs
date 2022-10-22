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
#include <coro_rpc/coro_rpc_server.hpp>
#include <coro_rpc/rpc_connection.hpp>

#include "doctest.h"
using namespace coro_rpc;
using namespace coro_rpc::internal;
TEST_CASE("test get_return_type in connection") {
  SUBCASE("no conn") {
    auto ret = get_return_type<false, int>();
    CHECK(ret == 0);
  }
  SUBCASE("return void") {
    static_assert(
        std::is_same_v<decltype(get_return_type<true, connection<void>>()),
                       void>);
  }
  SUBCASE("return std::string") {
    static_assert(std::is_same_v<
                  decltype(get_return_type<true, connection<std::string>>()),
                  std::string>);
  }
}