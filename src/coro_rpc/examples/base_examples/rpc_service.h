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
#ifndef CORO_RPC_RPC_API_HPP
#define CORO_RPC_RPC_API_HPP

#include <string>
#include <string_view>
#include <ylt/coro_rpc/coro_rpc_context.hpp>

std::string hello_world();
int A_add_B(int a, int b);
void hello_with_delay(coro_rpc::context<std::string> conn, std::string hello);
std::string echo(std::string_view sv);
void echo_with_attachment(coro_rpc::context<void> conn);
void echo_with_attachment2(coro_rpc::context<void> conn);
void return_error(coro_rpc::context<std::string> conn);
void rpc_with_state_by_tag(coro_rpc::context<std::string> conn);
async_simple::coro::Lazy<std::string> coro_echo(std::string_view sv);
async_simple::coro::Lazy<std::string> nested_echo(std::string_view sv);
class HelloService {
 public:
  std::string hello();
  void hello_with_delay(coro_rpc::context<std::string> conn, std::string hello);
};

#endif  // CORO_RPC_RPC_API_HPP
