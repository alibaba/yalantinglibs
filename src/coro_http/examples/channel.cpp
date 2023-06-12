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
#include "coro_io/channel.hpp"

#include <iostream>
#include <thread>

#include "async_simple/coro/Collect.h"
#include "async_simple/coro/Lazy.h"
#include "async_simple/coro/SyncAwait.h"
#include "cinatra/uri.hpp"
#include "coro_http/coro_http_client.h"
#include "coro_io/client_pool.hpp"
#include "coro_io/coro_io.hpp"

using namespace async_simple::coro;
using namespace coro_http;
using namespace std::chrono_literals;
using namespace coro_io;
std::atomic<int> qps, working_echo;
Lazy<void> test_async_channel(coro_io::channel<coro_http_client> &chan) {
  ++working_echo;
  for (int i = 0; i < 10000; ++i) {
    auto result = co_await chan.send_request(
        [&](coro_http_client &client,
            std::string_view host) -> Lazy<coro_http::resp_data> {
          co_return co_await client.async_get(std::string{host});
        });
    if (result.value().net_err) {
      break;
    }
    else {
      ++qps;
    }
  }
  --working_echo;
}

Lazy<void> qps_watcher() {
  using namespace std::chrono_literals;
  while (working_echo > 0) {
    co_await coro_io::sleep_for(1s);
    uint64_t cnt = qps.exchange(0);
    std::cout << "QPS:" << cnt << " working echo:" << working_echo << std::endl;
    std::cout
        << "free client for localhost: "
        << (coro_io::g_clients_pool<coro_http_client>()["http://www.baidu.com"])
               ->free_client_count()
        << std::endl;
    cnt = 0;
  }
}

int main() {
  std::vector<std::string> hosts;
  hosts.emplace_back("http://www.baidu.com");
  auto chan = coro_io::channel<coro_http_client>::create(
      hosts, coro_io::channel<coro_http_client>::channel_config{
                 .pool_config{.max_connection_ = 1000}});

  for (int i = 0, lim = std::thread::hardware_concurrency() * 10; i < lim; ++i)
    test_async_channel(chan).start([](auto &&) {
    });
  syncAwait(qps_watcher());
  std::cout << "OK" << std::endl;
  return 0;
}