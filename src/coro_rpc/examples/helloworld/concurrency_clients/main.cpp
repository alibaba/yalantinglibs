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
#include "asio_util/asio_coro_util.hpp"
#include "asio_util/io_context_pool.hpp"
#include "rpc_service/rpc_service.h"
using namespace coro_rpc;
using namespace async_simple::coro;
using namespace std::string_view_literals;

std::atomic<uint64_t> qps = 0;
/*!
 * \example helloworld/concurrency_clients.main.cpp
 * \brief demo for run concurrency clients
 */

Lazy<void> call_echo(asio::io_context& ioc) {
  while (true) {
    coro_rpc_client client(ioc);
    for (auto ec = co_await client.connect("127.0.0.1", "8801");
         ec != err_ok;) {
      std::cout << "connect failed." << std::endl;
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

Lazy<void> qps_watcher(asio::io_context& ioc) {
  using namespace std::chrono_literals;
  asio_util::period_timer timer(ioc);
  while (true) {
    timer.expires_after(1s);
    co_await timer.async_await();
    uint64_t cnt = qps.exchange(0);
    std::cout << "QPS:" << cnt << std::endl;
    cnt = 0;
  }
}

int main() {
  auto thread_cnt = std::thread::hardware_concurrency();
  asio_util::io_context_pool pool(thread_cnt);
  // total client cnt = thread_cnt * 20;
  for (int i = 0, lim = thread_cnt * 20; i < lim; ++i) {
    call_echo(pool.get_io_context()).start([](auto&&) {
    });
  }
  qps_watcher(pool.get_io_context()).start([](auto&&) {
  });
  pool.run();
  std::cout << "Done!" << std::endl;
  return 0;
}