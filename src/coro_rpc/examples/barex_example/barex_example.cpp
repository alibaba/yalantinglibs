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
#include <chrono>
#include <memory>
#include <thread>

#include "async_simple/coro/SyncAwait.h"
#include "ylt/coro_io/barex/barex_device.hpp"
#include "ylt/coro_io/barex/barex_server_acceptor.hpp"
#include "ylt/coro_io/barex/barex_socket.hpp"
#include "ylt/coro_io/coro_io.hpp"
#include "ylt/coro_io/server_acceptor.hpp"
#include "ylt/coro_rpc/coro_rpc_client.hpp"
#include "ylt/coro_rpc/coro_rpc_server.hpp"
#include "ylt/coro_rpc/impl/default_config/coro_rpc_config.hpp"
#include "ylt/easylog.hpp"
using namespace coro_rpc;
using namespace async_simple::coro;
using namespace std::chrono_literals;
std::string_view echo(std::string_view data) {
  ELOG_INFO << "echo recv data, bytes = " << data.size();
  return data;
}

// The basic example about how to start a rpc connection over ibverbs.
void basic_example() {
  coro_rpc_client client;
  std::vector<std::unique_ptr<coro_io::server_acceptor_base>> acceptors;
  acceptors.emplace_back(std::make_unique<coro_io::barex_server_acceptor>(
      9001, coro_io::barex_socket_t::config_t{
                .buffer_size = 256 * 1024,
                .dev = coro_io::get_global_barex_device()}));
  coro_rpc_server server(config_t{}, std::move(acceptors));
  server.register_handler<echo>();
  auto future = server.async_start();
  if (future.hasResult()) {
    ELOG_ERROR << future.result().value().message();
    return;
  }
  [[maybe_unused]] bool is_ok = client.init_barex(
      coro_io::barex_socket_t::config_t{.buffer_size = 256 * 1024});
  auto ec = syncAwait(client.connect(std::string{server.address()} + ":" +
                                     std::to_string(server.port())));
  if (ec) {
    ELOG_ERROR << ec.message();
    return;
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
  return;
}

int main() {
  easylog::logger<>::instance().set_min_severity(easylog::Severity::TRACE);
  easylog::logger<>::instance().set_async(false);
  basic_example();
  return 0;
}