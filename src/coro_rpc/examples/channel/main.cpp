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
#include <atomic>
#include <chrono>
#include <climits>
#include <coro_rpc/coro_rpc_client.hpp>
#include <cstdlib>
#include <memory>
#include <thread>

#include "asio/io_context.hpp"
#include "asio/steady_timer.hpp"
#include "async_simple/coro/Collect.h"
#include "async_simple/coro/Lazy.h"
#include "coro_io/channel.hpp"
#include "coro_io/client_pool.hpp"
#include "coro_io/coro_io.hpp"
#include "coro_io/io_context_pool.hpp"
#include "coro_rpc/coro_rpc/coro_rpc_client.hpp"
using namespace coro_rpc;
using namespace async_simple::coro;
using namespace std::string_view_literals;
using namespace std::chrono_literals;
std::string echo(std::string_view sv);
std::atomic<uint64_t> qps = 0;

std::atomic<uint64_t> working_echo = 0;
/*!
 * \example helloworld/concurrency_clients.main.cpp
 * \brief demo for run concurrency clients
 */

Lazy<void> call_echo(coro_io::channel<coro_rpc_client> &channel, int cnt) {
  while (true) {
    ++working_echo;
    for (int i = 0; i < cnt; ++i) {
      auto res = co_await channel.send_request(
          [](coro_rpc_client &client, std::string_view hostname) -> Lazy<void> {
            auto res = co_await client.call<echo>("Hello world!");
            if (!res.has_value()) {
              std::cout << "coro_rpc err: \n";
              co_return;
            }
            if (res.value() != "Hello world!"sv) {
              std::cout << "err echo resp: \n";
              co_return;
            }
            ++qps;
            co_return;
          });
      if (!res) {
        std::cout << "client pool err: connect failed.\n";
      }
    }
    --working_echo;
    co_await coro_io::sleep_for(30s);
  }
}

Lazy<void> qps_watcher() {
  using namespace std::chrono_literals;
  auto &clients = coro_io::g_clients_pool<coro_rpc_client>();
  while (working_echo > 0) {
    co_await coro_io::sleep_for(1s);
    uint64_t cnt = qps.exchange(0);
    std::cout << "QPS:" << cnt << " working echo:" << working_echo << std::endl;
    std::cout << "free client for localhost: "
              << (co_await clients["localhost:8801"])->free_client_count()
              << std::endl;
    std::cout << "free client for 127.0.0.1: "
              << (co_await clients["127.0.0.1:8801"])->free_client_count()
              << std::endl;
    cnt = 0;
  }
}
auto hosts = std::vector<std::string>{
    {std::string{"127.0.0.1:8801"}, std::string{"localhost:8801"}}};
Lazy<void> start() {
  auto worker_cnt = std::thread::hardware_concurrency() * 20;
  try {
    auto chan = co_await coro_io::channel<coro_rpc_client>::create(hosts);

    std::vector<async_simple::coro::Lazy<void>> works;
    works.reserve(worker_cnt);
    for (int i = 0; i < worker_cnt; ++i) {
      works.emplace_back(call_echo(chan, 10000));
    }
    co_await collectAll(std::move(works));
  } catch (const std::exception &e) {
    std::cout << e.what() << std::endl;
  }
}

int main() {
  start().start([](auto &&) {
  });
  syncAwait(qps_watcher());
  std::cout << "Done!" << std::endl;
  return 0;
}