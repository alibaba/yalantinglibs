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

#include <atomic>
#include <chrono>
#include <climits>
#include <cstdlib>
#include <locale>
#include <memory>
#include <thread>
#include <ylt/coro_io/client_pool.hpp>
#include <ylt/coro_io/coro_io.hpp>
#include <ylt/coro_rpc/coro_rpc_client.hpp>

#include "async_simple/coro/Collect.h"
#include "async_simple/coro/Lazy.h"
#include "async_simple/coro/SyncAwait.h"
#include "ylt/coro_io/io_context_pool.hpp"
#include "ylt/easylog.hpp"
std::string echo(std::string_view sv);

using namespace coro_rpc;
using namespace async_simple::coro;
using namespace std::string_view_literals;

std::atomic<uint64_t> qps = 0;

std::atomic<uint64_t> working_echo = 0;
std::atomic<uint64_t> busy_echo = 0;
struct guard {
  guard(std::atomic<uint64_t> &ref) : ref(ref) { ++ref; }
  ~guard() { --ref; }
  std::atomic<uint64_t> &ref;
};
/*!
 * \example helloworld/concurrency_clients.main.cpp
 * \brief demo for run concurrency clients
 */

Lazy<std::vector<std::chrono::microseconds>> call_echo(
    coro_io::client_pool<coro_rpc_client> &client_pool, int cnt) {
  std::vector<std::chrono::microseconds> result;
  result.reserve(cnt);
  ++working_echo;
  for (int i = 0; i < cnt; ++i) {
    auto tp = std::chrono::steady_clock::now();
    auto res = co_await client_pool.send_request(
        [=](coro_rpc_client &client) -> Lazy<void> {
          guard g{busy_echo};
          if (client.has_closed()) {
            co_return;
          }
          auto res = co_await client.call<echo>("Hello world!");
          if (!res.has_value()) {
            ELOG_ERROR << "coro_rpc err: \n" << res.error().msg;
            client.close();
            co_return;
          }
          if (res.value() != "Hello world!"sv) {
            ELOG_ERROR << "err echo resp: \n" << res.value();
            co_return;
          }
          ++qps;
          co_return;
        });
    if (!res) {
      ELOG_ERROR << "client pool err: connect failed.\n";
    }
    else {
      result.push_back(std::chrono::duration_cast<std::chrono::microseconds>(
          std::chrono::steady_clock::now() - tp));
    }
  }
  co_return std::move(result);
}

Lazy<void> qps_watcher(coro_io::client_pool<coro_rpc_client> &clients) {
  using namespace std::chrono_literals;
  while (working_echo > 0) {
    co_await coro_io::sleep_for(1s);
    uint64_t cnt = qps.exchange(0);
    std::cout << "QPS:" << cnt
              << " free connection: " << clients.free_client_count()
              << " working echo:" << working_echo << " busy echo:" << busy_echo
              << std::endl;
    cnt = 0;
  }
}
std::vector<std::chrono::microseconds> result;
void latency_watcher() {
  std::sort(result.begin(), result.end());
  auto arr = {0.1, 0.3, 0.5, 0.7, 0.9, 0.95, 0.99, 0.999, 0.9999, 0.99999, 1.0};
  for (auto e : arr) {
    std::cout
        << (e * 100) << "% request finished in:"
        << result[std::max<std::size_t>(0, result.size() * e - 1)].count() /
               1000.0
        << "ms" << std::endl;
  }
}
int main() {
  auto thread_cnt = std::thread::hardware_concurrency();
  auto client_pool = coro_io::client_pool<coro_rpc_client>::create(
      "localhost:8801",
      coro_io::client_pool<coro_rpc_client>::pool_config{
          .max_connection = thread_cnt * 20,
          .client_config = {.timeout_duration = std::chrono::seconds{50}}});

  auto finish_executor = coro_io::get_global_block_executor();
  for (int i = 0, lim = thread_cnt * 20; i < lim; ++i) {
    call_echo(*client_pool, 10000).start([finish_executor](auto &&res) {
      finish_executor->schedule([res = std::move(res.value())] {
        result.insert(result.end(), res.begin(), res.end());
        --working_echo;
      });
    });
  }
  syncAwait(qps_watcher(*client_pool));
  latency_watcher();
  std::cout << "Done!" << std::endl;
  return 0;
}