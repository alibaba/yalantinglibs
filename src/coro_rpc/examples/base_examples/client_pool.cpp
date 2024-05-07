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
#include <cstdint>
#include <cstdlib>
#include <locale>
#include <memory>
#include <mutex>
#include <optional>
#include <thread>
#include <ylt/coro_io/client_pool.hpp>
#include <ylt/coro_io/coro_io.hpp>
#include <ylt/coro_rpc/coro_rpc_client.hpp>

#include "async_simple/coro/Collect.h"
#include "async_simple/coro/Lazy.h"
#include "async_simple/coro/Mutex.h"
#include "async_simple/coro/SyncAwait.h"
#include "ylt/coro_io/io_context_pool.hpp"
#include "ylt/coro_rpc/impl/coro_rpc_client.hpp"
#include "ylt/coro_rpc/impl/errno.h"
#include "ylt/coro_rpc/impl/expected.hpp"
#include "ylt/easylog.hpp"
std::string echo(std::string_view sv);

constexpr auto thread_cnt = 96*2;
constexpr auto request_cnt= 200000;
using namespace coro_rpc;
using namespace async_simple::coro;
using namespace std::string_view_literals;

auto finish_executor = coro_io::get_global_block_executor();
std::atomic<uint64_t> working_echo = 0;
std::atomic<bool> stop=false;
struct guard {
  guard(std::atomic<uint64_t> &ref) : ref(ref) { ++ref; }
  ~guard() { --ref; }
  std::atomic<uint64_t> &ref;
};
/*!
 * \example helloworld/concurrency_clients.main.cpp
 * \brief demo for run concurrency clients
 */

std::vector<std::unique_ptr<coro_rpc_client>> clients;

std::atomic<uint64_t>& get_qps(int id) {
  static std::atomic<uint64_t> ar[thread_cnt*8];
  return ar[id*8];
}

int& get_cnt(int id) {
  static int ar[thread_cnt*16];
  return ar[id*16];
}
int& get_flag(int id) {
  static int ar[thread_cnt*16];
  return ar[id*16];
}
std::vector<std::chrono::microseconds>& get_result(int id) {
  static std::vector<std::chrono::microseconds> ar[thread_cnt*3];
  return ar[id*3];
}
int cnt_max=10;
Lazy<void> send(int id) {
  auto &cli=*clients[id];
  auto& qps=get_qps(id);
  auto &cnt=get_cnt(id);
  auto &result=get_result(id);
  ++working_echo;
  for (;result.size()<request_cnt;) {
    auto tp = std::chrono::steady_clock::now();
    auto res_ = co_await cli.send_request<echo>("Hello world!");
    if (!res_.has_value()) {
      ELOG_ERROR << "coro_rpc err: \n" << res_.error().msg;
      continue;
    }
    res_.value().start([id,&qps,&cnt,&result,old_tp=tp](auto&&res) {
      auto&res1=res.value();
      if (!res1.has_value()) {
        ELOG_ERROR << "coro_rpc err: \n" << res1.error().msg;
      }
      else {
        ++qps;
        result.push_back(std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now()-old_tp));
        auto tmp=cnt--;
        if (tmp==cnt_max) {
          get_flag(id)=true;
        }
        else if (tmp==cnt_max/2 && get_flag(id)) {
            get_flag(id)=false;
            send(id).start([](auto&& res){
          });
        }
      }
    });
    auto cnt_tmp=++cnt;
    if (cnt_tmp==cnt_max) {
      break;
    }
  }
  --working_echo;
  co_return;
}

Lazy<void> qps_watcher() {
  using namespace std::chrono_literals;
  do {
    co_await coro_io::sleep_for(1s);
    uint64_t cnt = 0;
    for (int i=0;i<thread_cnt;++i)
    {
      auto & qps=get_qps(i);
      uint64_t tmp=qps.exchange(0);
      cnt+=tmp;
    }
    std::cout << "QPS:" << cnt
             // << " free connection: " << clients.free_client_count()
              << " working echo:" << working_echo
              << std::endl;
  } while (working_echo>0);
}
std::vector<std::chrono::microseconds> result;
void latency_watcher() {
  result.reserve(request_cnt*thread_cnt);
  for (int i=0;i<thread_cnt;++i) {
    auto&res = get_result(i);
    result.insert(result.end(), res.begin(), res.end());
  }
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
  for (int i=0;i<thread_cnt;++i) {
    clients.emplace_back(std::make_unique<coro_rpc_client>(coro_io::get_global_executor()->get_asio_executor()));
    syncAwait(clients.back()->connect("localhost:8801"));
    get_result(i).reserve(request_cnt);
  }
  for (int i = 0, lim = thread_cnt; i < lim; ++i) {
    send(i).via(&clients[i]->get_executor()).start([](auto &&res) {
    });
  }
  syncAwait(qps_watcher());
  latency_watcher();
  std::cout << "Done!" << std::endl;
  return 0;
}