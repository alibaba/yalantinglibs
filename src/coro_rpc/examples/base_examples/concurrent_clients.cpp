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
#include <async_simple/coro/Lazy.h>
#include <async_simple/coro/Sleep.h>

#include <asio/io_context.hpp>
#include <asio/steady_timer.hpp>
#include <atomic>
#include <chrono>
#include <climits>
#include <cstdlib>
#include <system_error>
#include <thread>
#include <ylt/coro_io/client_pool.hpp>
#include <ylt/coro_io/coro_io.hpp>
#include <ylt/coro_rpc/coro_rpc_client.hpp>

#include "ylt/coro_rpc/impl/coro_rpc_client.hpp"
std::string echo(std::string_view sv);
using namespace coro_rpc;
using namespace async_simple::coro;
using namespace std::string_view_literals;
using namespace std::chrono_literals;
std::atomic<uint64_t> qps = 0;

std::atomic<uint64_t> working_echo = 0;
/*!
 * \example helloworld/concurrency_clients.main.cpp
 * \brief demo for run concurrency clients
 */

Lazy<void> call_echo(int cnt) {
  ++working_echo;
  coro_rpc_client client;
  auto ec = co_await client.connect("localhost:8801");
  for (int i = 0; i < 3 && !ec; ++i) {
    co_await coro_io::sleep_for(rand() % 10000 * 1ms);
    ec = co_await client.reconnect("localhost:8801");
  }
  if (!ec) {
    for (int i = 0; i < cnt; ++i) {
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
    }
  }
  else {
    std::cout << "connect failed \n";
  }
  --working_echo;
}

Lazy<void> qps_watcher() {
  using namespace std::chrono_literals;
  while (working_echo > 0) {
    co_await coro_io::sleep_for(1s);
    uint64_t cnt = qps.exchange(0);
    std::cout << "QPS:" << cnt << " working echo:" << working_echo << std::endl;
    cnt = 0;
  }
}

int main() {
  auto thread_cnt = std::thread::hardware_concurrency();
  for (int i = 0, lim = thread_cnt * 20; i < lim; ++i) {
    call_echo(100000).start([](auto &&) {
    });
  }
  syncAwait(qps_watcher());
  std::cout << "Done!" << std::endl;
  return 0;
}