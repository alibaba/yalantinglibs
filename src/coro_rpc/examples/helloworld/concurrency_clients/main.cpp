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
#include <coro_rpc/coro_rpc_client.hpp>
#include <thread>

#include "asio/io_context.hpp"
#include "asio/steady_timer.hpp"
#include "coro_io/coro_io.hpp"
#include "coro_io/io_context_pool.hpp"
#include "rpc_service/rpc_service.h"
using namespace coro_rpc;
using namespace async_simple::coro;
using namespace std::string_view_literals;

std::atomic<uint64_t> qps = 0;
/*!
 * \example helloworld/concurrency_clients.main.cpp
 * \brief demo for run concurrency clients
 */

Lazy<void> call_echo() {
  while (true) {
    coro_rpc_client client(*coro_io::get_global_executor());
    for (auto ec = co_await client.connect("127.0.0.1", "8801");
         ec != std::errc{};) {
      std::cout << "connect failed." << std::endl;
      ec = co_await client.reconnect("127.0.0.1", "8801");
    }
    while (true) {
      auto ret = co_await client.call<echo>("Hello world!"sv);
      if (!ret) [[unlikely]] {
        std::cout << "coro_rpc err: " << ret.error().msg << std::endl;
        break;
      }

      if (ret.value() != "Hello world!"sv) {
        std::cout << "err echo resp: " << ret.value() << std::endl;
        break;
      }
      ++qps;
    }
  }
}

Lazy<void> qps_watcher() {
  using namespace std::chrono_literals;
  for (int i = 0; i < 30; ++i) {
    co_await coro_io::sleep_for(1s);
    uint64_t cnt = qps.exchange(0);
    std::cout << "QPS:" << cnt << std::endl;
    cnt = 0;
  }
}

int main() {
  auto thread_cnt = std::thread::hardware_concurrency();
  // total client cnt = thread_cnt * 20;
  for (int i = 0, lim = thread_cnt * 20; i < lim; ++i) {
    call_echo().start([](auto&&) {
    });
  }
  syncAwait(qps_watcher());
  std::cout << "Done!" << std::endl;
  return 0;
}