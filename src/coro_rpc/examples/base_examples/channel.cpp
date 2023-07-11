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
#include "ylt/coro_io/channel.hpp"

#include <async_simple/coro/Collect.h>
#include <async_simple/coro/Lazy.h>

#include <asio/io_context.hpp>
#include <asio/steady_timer.hpp>
#include <atomic>
#include <chrono>
#include <climits>
#include <cstdlib>
#include <memory>
#include <thread>
#include <ylt/coro_io/client_pool.hpp>
#include <ylt/coro_io/coro_io.hpp>
#include <ylt/coro_rpc/coro_rpc_client.hpp>
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

Lazy<void> call_echo(std::shared_ptr<coro_io::channel<coro_rpc_client>> channel,
                     int cnt) {
  while (true) {
    ++working_echo;
    for (int i = 0; i < cnt; ++i) {
      auto res = co_await channel->send_request(
          [](coro_rpc_client &client, std::string_view hostname) -> Lazy<void> {
            auto res = co_await client.call<echo>("Hello world!");
            if (!res.has_value()) {
              std::cout << "coro_rpc err: \n" << res.error().msg;
              co_return;
            }
            if (res.value() != "Hello world!"sv) {
              std::cout << "err echo resp: \n" << res.value();
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
              << (clients["localhost:8801"])->free_client_count() << std::endl;
    std::cout << "free client for 127.0.0.1: "
              << (clients["127.0.0.1:8801"])->free_client_count() << std::endl;
    cnt = 0;
  }
}

int main() {
  auto hosts =
      std::vector<std::string_view>{{"127.0.0.1:8801", "localhost:8801"}};
  auto worker_cnt = std::thread::hardware_concurrency() * 20;
  auto chan = coro_io::channel<coro_rpc_client>::create(
      hosts, coro_io::channel<coro_rpc_client>::channel_config{
                 .pool_config{.max_connection = worker_cnt}});
  auto chan_ptr = std::make_shared<decltype(chan)>(std::move(chan));
  for (int i = 0; i < worker_cnt; ++i) {
    call_echo(chan_ptr, 10000).start([](auto &&) {
    });
  }
  syncAwait(qps_watcher());
  std::cout << "Done!" << std::endl;
  return 0;
}