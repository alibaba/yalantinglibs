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

#include "rpc_service/rpc_service.h"
using namespace coro_rpc;
using namespace async_simple::coro;
using namespace std::string_literals;
/*!
 * \example helloworld/client.main.cpp
 * \brief helloworld example client code
 */

Lazy<void> show_rpc_call(coro_rpc_client &client) {
  [[maybe_unused]] auto ec = co_await client.connect("127.0.0.1", "8801");
  assert(ec == err_ok);
  auto ret = co_await client.call<hello_world>();
  if (!ret) {
    std::cout << "err: " << ret.error().msg << std::endl;
  }
  assert(ret.value() == "hello world"s);

  ret = co_await client.call<echo>("hello coro_rpc");
  if (!ret) {
    std::cout << "err: " << ret.error().msg << std::endl;
  }
  assert(ret.value() == "hello coro_rpc"s);

  ret = co_await client.call<hello_with_delay>();
  if (!ret) {
    std::cout << "err: " << ret.error().msg << std::endl;
  }
  assert(ret.value() == "hello coro_rpc"s);

  ret = co_await client.call<&HelloService::hello>();
  if (!ret) {
    std::cout << "err: " << ret.error().msg << std::endl;
  }
  assert(ret.value() == "hello"s);
}

int main() {
  coro_rpc_client client;
  syncAwait(show_rpc_call(client));

  std::cout << "Done!" << std::endl;
  return 0;
}