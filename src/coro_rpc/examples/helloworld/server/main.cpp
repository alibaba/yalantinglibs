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

#include "rpc_service/rpc_service.h"
using namespace coro_rpc;
using namespace async_simple;
using namespace async_simple::coro;
int main() {
  // start rpc server
  coro_rpc_server server(2, 8801);

  // regist normal function for rpc
  server.regist_handler<hello_world, echo, hello_with_delay>();

  // regist member function for rpc
  HelloService hello_service;
  server.regist_handler<&HelloService::hello>(&hello_service);

  auto ec = server.start();
  return ec == std::errc{};
}