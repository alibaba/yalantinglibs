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
#include <ylt/coro_rpc/coro_rpc_client.hpp>

#include "rpc_service.h"
#include "ylt/coro_rpc/impl/errno.h"
#include "ylt/coro_rpc/impl/protocol/coro_rpc_protocol.hpp"
using namespace coro_rpc;
using namespace async_simple::coro;
using namespace std::string_literals;
/*!
 * \example helloworld/client.main.cpp
 * \brief helloworld example client code
 */

Lazy<void> show_rpc_call() {
  coro_rpc_client client;

  [[maybe_unused]] auto ec = co_await client.connect("127.0.0.1", "8801");
  assert(!ec);

  auto ret = co_await client.call<echo>("hello");
  assert(ret.value() == "hello");

  ret = co_await client.call<coroutine_echo>("42");
  assert(ret.value() == "42");

  ret = co_await client.call<async_echo_by_callback>("hi");
  assert(ret.value() == "hi");

  ret = co_await client.call<async_echo_by_coroutine>("hey");
  assert(ret.value() == "hey");

  client.set_req_attachment("This is attachment.");
  auto ret_void = co_await client.call<echo_with_attachment>();
  assert(client.get_resp_attachment() == "This is attachment.");

  ret = co_await client.call<nested_echo>("nested_echo"s);
  assert(ret.value() == "nested_echo"s);

  ret = co_await client.call<&HelloService::hello>();
  assert(ret.value() == "HelloService::hello"s);

  ret_void = co_await client.call<get_ctx_info>();
  assert(ret_void);

  // TODO: fix return error
  // ret_void = co_await client.call<return_error_by_context>();

  // assert(ret.error().code.val() == 404);
  // assert(ret.error().msg == "404 Not Found.");

  // ret_void = co_await client.call<return_error_by_exception>();

  // assert(ret.error().code.val() == 404);

  ret = co_await client.call<rpc_with_state_by_tag>();
  assert(ret.value() == "1");
  ret = co_await client.call<rpc_with_state_by_tag>();
  assert(ret.value() == "2");
  ret = co_await client.call<rpc_with_state_by_tag>();
  assert(ret.value() == "3");
}

int main() {
  try {
    syncAwait(show_rpc_call());
    std::cout << "Done!" << std::endl;
  } catch (const std::exception& e) {
    std::cout << "Error:" << e.what() << std::endl;
  }
  return 0;
}