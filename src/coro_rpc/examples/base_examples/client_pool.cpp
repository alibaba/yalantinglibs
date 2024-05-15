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
#include "ylt/easylog/record.hpp"
std::string echo(std::string_view sv);

constexpr unsigned thread_cnt = 1920;
constexpr auto request_cnt = 1000;
using namespace coro_rpc;
using namespace async_simple::coro;
using namespace std::string_view_literals;

auto finish_executor = coro_io::get_global_block_executor();
std::atomic<uint64_t> working_echo = 0;
std::atomic<bool> stop = false;
struct guard {
  guard(std::atomic<uint64_t>& ref) : ref(ref) { ++ref; }
  ~guard() { --ref; }
  std::atomic<uint64_t>& ref;
};
/*!
 * \example helloworld/concurrency_clients.main.cpp
 * \brief demo for run concurrency clients
 */

std::vector<std::unique_ptr<coro_rpc_client>> clients;

std::atomic<uint32_t> rr=0;
auto st=std::min(thread_cnt,std::thread::hardware_concurrency());
std::atomic<uint64_t> tid=0;
uint64_t& get_tid() {
  thread_local static uint64_t mytid=tid++;
  return mytid;
}

// coro_rpc_client& get_client() {
//   thread_local static uint32_t rr=get_tid();
//   coro_rpc_client* cli;
//   cli = clients[rr%thread_cnt].get();
//   rr+=st;
//   if (rr>thread_cnt) {
//     rr%=st;
//   }
//   return *cli;
// }

coro_rpc_client& get_client() {
  coro_rpc_client* cli;
  auto v=rr.fetch_add(101);
  unsigned min_sz=UINT32_MAX;
  v%=thread_cnt;
  for (int i=0;min_sz>0 && i<=0;++i) {
    auto sz=clients[v]->get_pipeline_size();
    if (min_sz > sz) {
      cli = clients[v].get();
      min_sz = sz;
    }
    v+=1;
    v%=thread_cnt;
  }
  return *cli;
}
std::atomic<int32_t> qps=0;
Lazy<std::vector<std::chrono::microseconds>> send() {
  std::vector<std::chrono::microseconds> result;
  result.reserve(request_cnt);
  auto tp = std::chrono::steady_clock::now();
  ++working_echo;
  int id=0;
  for (int i=0;i<request_cnt;++i) {
    auto &cli=get_client();
    auto res_ = co_await cli.send_request<echo>("Hello world!");
    if (!res_.has_value()) {
      ELOG_ERROR << "coro_rpc err: \n" << res_.error().msg;
      continue;
    }
    auto res1 = co_await res_.value();
    if (!res1.has_value()) {
      ELOG_ERROR << "coro_rpc err: \n" << res1.error().msg;
      break;
    }
    qps.fetch_add(1,std::memory_order::release);
    auto old_tp=tp;
    tp= std::chrono::steady_clock::now();
    result.push_back(std::chrono::duration_cast<std::chrono::microseconds>(
        tp - old_tp));
  }
  co_return std::move(result);
}

Lazy<void> qps_watcher() {
  using namespace std::chrono_literals;
  do {
    co_await coro_io::sleep_for(1s);
    uint64_t cnt = qps.exchange(0);
    std::cout << "QPS:"
              << cnt
              // << " free connection: " << clients.free_client_count()
              << " working echo:" << working_echo << std::endl;
  } while (working_echo > 0);
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
  for (int i = 0; i < std::thread::hardware_concurrency(); ++i) {
    coro_io::get_global_executor()->schedule([i](){
      get_tid()=i;
    });
  }
  for (int i = 0; i < thread_cnt; ++i) {
    clients.emplace_back(std::make_unique<coro_rpc_client>(
        coro_io::get_global_executor()->get_asio_executor()));
    syncAwait(clients.back()->connect("localhost:8801"));
  }
  auto executor = coro_io::get_global_block_executor();
 // auto e = coro_io::get_global_executor();
  for (int i = 0, lim = thread_cnt*10; i < lim; ++i) {
    coro_io::get_global_executor()->schedule([=](){send().start([executor](auto&& res) {
      executor->schedule([res = std::move(res.value())]() mutable{
        result.insert(result.end(), res.begin(), res.end());
        --working_echo;
      });
    });});
  }
  syncAwait(qps_watcher());
  latency_watcher();
  std::cout << "Done!" << std::endl;
  return 0;
}