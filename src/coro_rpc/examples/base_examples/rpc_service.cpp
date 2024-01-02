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
#include "rpc_service.h"

#include <chrono>
#include <cstdint>
#include <thread>
#include <ylt/coro_rpc/coro_rpc_client.hpp>
#include <ylt/easylog.hpp>

#include "ylt/coro_rpc/impl/errno.h"

using namespace coro_rpc;

std::string hello_world() {
  ELOGV(INFO, "call helloworld");
  return "hello_world";
}

int A_add_B(int a, int b) {
  ELOGV(INFO, "call A+B");
  return a + b;
}

void echo_with_attachment(coro_rpc::context<void> conn) {
  ELOGV(INFO, "call echo_with_attachment");
  std::string str = conn.release_request_attachment();
  conn.set_response_attachment(std::move(str));
  conn.response_msg();
}

void echo_with_attachment2(coro_rpc::context<void> conn) {
  ELOGV(INFO, "call echo_with_attachment2");
  std::string_view str = conn.get_request_attachment();
  // The live time of attachment is same as coro_rpc::context
  conn.set_response_attachment([str, conn] {
    return str;
  });
  conn.response_msg();
}

std::string echo(std::string_view sv) { return std::string{sv}; }

async_simple::coro::Lazy<std::string> coro_echo(std::string_view sv) {
  ELOGV(INFO, "call coro_echo");
  co_await coro_io::sleep_for(std::chrono::milliseconds(100));
  ELOGV(INFO, "after sleep for a while");
  co_return std::string{sv};
}

void hello_with_delay(context</*response type:*/ std::string> conn,
                      std::string hello) {
  ELOGV(INFO, "call HelloServer hello_with_delay");
  // create a new thread
  std::thread([conn = std::move(conn), hello = std::move(hello)]() mutable {
    // do some heavy work in this thread that won't block the io-thread,
    std::cout << "running heavy work..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds{1});
    // Remember response before connection destruction! Or the connect will
    // be closed.
    conn.response_msg(hello);
  }).detach();
}

async_simple::coro::Lazy<std::string> nested_echo(std::string_view sv) {
  ELOGV(INFO, "start nested echo");
  coro_rpc::coro_rpc_client client(co_await coro_io::get_current_executor());
  [[maybe_unused]] auto ec = co_await client.connect("127.0.0.1", "8802");
  assert(!ec);
  ELOGV(INFO, "connect another server");
  auto ret = co_await client.call<echo>(sv);
  assert(ret.value() == sv);
  ELOGV(INFO, "get echo result from another server");
  co_return std::string{sv};
}

std::string HelloService::hello() {
  ELOGV(INFO, "call HelloServer::hello");
  return "HelloService::hello";
}

void HelloService::hello_with_delay(
    coro_rpc::context</*response type:*/ std::string> conn, std::string hello) {
  ELOGV(INFO, "call HelloServer::hello_with_delay");
  std::thread([conn = std::move(conn), hello = std::move(hello)]() mutable {
    conn.response_msg("HelloService::hello_with_delay");
  }).detach();
  return;
}

void return_error(coro_rpc::context<std::string> conn) {
  conn.response_error(coro_rpc::err_code{404}, "404 Not Found.");
}
void rpc_with_state_by_tag(coro_rpc::context<std::string> conn) {
  if (!conn.tag().has_value()) {
    conn.tag() = uint64_t{0};
  }
  auto &cnter = std::any_cast<uint64_t &>(conn.tag());
  ELOGV(INFO, "call count: %d", ++cnter);
  conn.response_msg(std::to_string(cnter));
}