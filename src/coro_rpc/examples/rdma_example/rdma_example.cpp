/*
 * Copyright (c) 2025, Alibaba Group Holding Limited;
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
#include "async_simple/coro/SyncAwait.h"
#include "ylt/coro_rpc/coro_rpc_client.hpp"
#include "ylt/coro_rpc/coro_rpc_server.hpp"
#include "ylt/easylog.hpp"
#include "ylt/easylog/record.hpp"
using namespace coro_rpc;
using namespace async_simple::coro;
std::string_view echo(std::string_view data) { return data; }
int main() {
  easylog::logger<>::instance().set_min_severity(easylog::Severity::INFO);
  coro_rpc_client client;
  coro_rpc_server server;
  server.init_ibv();
  server.register_handler<echo>();
  auto future = server.async_start();
  if (future.hasResult()) {
    ELOG_ERROR << future.result().value().message();
    return 0;
  }
  [[maybe_unused]] bool is_ok = client.init_ibv();
  auto ec = syncAwait(client.connect(std::string{server.address()} + ":" +
                                     std::to_string(server.port())));
  if (ec) {
    ELOG_ERROR << ec.message();
    return 0;
  }
  std::string data(1024 * 1024 * 10, 'A');
  auto result = syncAwait(client.call<echo>(data));
  if (result != data) {
    ELOG_ERROR << "echo data err";
  }
  else {
    ELOG_INFO << "echo ok!";
  }
  server.stop();
  return 0;
}