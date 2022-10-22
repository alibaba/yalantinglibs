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
#include <memory>
#include <variant>
#include <vector>

namespace coro_rpc {

class async_connection;
class coro_connection;
using rpc_conn = std::variant<std::shared_ptr<async_connection>,
                              std::shared_ptr<coro_connection>>;
namespace internal {
std::pair<std::errc, std::vector<char>> route(auto handler,
                                              std::string_view data,
                                              rpc_conn conn);
async_simple::coro::Lazy<std::pair<std::errc, std::vector<char>>> route_coro(
    auto handler, std::string_view data, rpc_conn conn);

std::function<std::pair<std::errc, std::vector<char>>(std::string_view,
                                                      rpc_conn &)>
    *get_handler(std::string_view data);

std::function<async_simple::coro::Lazy<std::pair<std::errc, std::vector<char>>>(
    std::string_view, rpc_conn &)>
    *get_coro_handler(std::string_view data);
}  // namespace internal
}  // namespace coro_rpc