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
#include "rpc_service/rpc_service.h"

#include <easylog/easylog.h>

#include <chrono>
#include <thread>

#include "asio_util/asio_coro_util.hpp"
#include "async_simple/coro/Sleep.h"

using namespace coro_rpc;

std::string hello_world() {
  ELOGV(INFO, "call helloworld");
  return "hello_world";
}

int A_add_B(int a, int b) {
  ELOGV(INFO, "call A+B");
  return a + b;
}

std::string_view echo(std::string_view sv) { return sv; }

async_simple::coro::Lazy<std::string> coro_echo(std::string_view sv) {
  ELOGV(INFO, "call coro_echo");
  co_await async_simple::coro::sleep(std::chrono::milliseconds(100));
  ELOGV(INFO, "after sleep for a while");
  co_return std::string{sv};
}

void hello_with_delay(connection</*response type:*/ std::string> conn) {
  ELOGV(INFO, "call HelloServer hello_with_delay");
  std::thread([conn]() mutable {
    conn.response_msg("hello_with_delay");
  }).detach();
}

std::string HelloService::hello() {
  ELOGV(INFO, "call HelloServer::hello");
  return "HelloService::hello";
}

void HelloService::hello_with_delay(
    coro_rpc::connection</*response type:*/ std::string> conn) {
  ELOGV(INFO, "call HelloServer::hello_with_delay");
  std::thread([conn]() mutable {
    conn.response_msg("HelloService::hello_with_delay");
  }).detach();
  return;
}
