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
#ifndef CORO_RPC_RPC_API_HPP
#define CORO_RPC_RPC_API_HPP

#include <string>

#include <ylt/coro_rpc/rpc_connection.hpp>

std::string hello_world();
std::string echo(std::string str);
void hello_with_delay(coro_rpc::connection<std::string> conn);

class HelloService {
 public:
  std::string hello();

 private:
};

#endif  // CORO_RPC_RPC_API_HPP
