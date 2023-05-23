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
#include <memory>
#include <thread>

#include "Monster.h"
#include "Rect.h"
#include "ValidateRequest.h"
#include "asio/associated_executor.hpp"
#include "async_simple/coro/Lazy.h"
#include "coro_io/io_context_pool.hpp"
#include "coro_rpc/rpc_context.hpp"

inline asio_util::io_context_pool pool(std::thread::hardware_concurrency());

inline std::string echo_4B(const std::string &str) { return str; }
inline std::string echo_50B(const std::string &str) { return str; }
inline std::string echo_100B(const std::string &str) { return str; }
inline std::string echo_500B(const std::string &str) { return str; }
inline std::string echo_1KB(const std::string &str) { return str; }
inline std::string echo_5KB(const std::string &str) { return str; }
inline std::string echo_10KB(const std::string &str) { return str; }

inline std::vector<int> array_1K_int(std::vector<int> ar) { return ar; }

inline std::vector<std::string> array_1K_str_4B(std::vector<std::string> ar) {
  return ar;
}

inline std::vector<rect> array_1K_rect(std::vector<rect> ar) { return ar; }

auto many_argument(auto... args) { return (args + ...); }

inline void monsterFunc(Monster monster) { return; }

inline void ValidateRequestFunc(ValidateRequest req) { return; }

inline void heavy_calculate(coro_rpc::context<int> conn, int a) {
  [](int a, coro_rpc::context<int> conn) -> async_simple::coro::Lazy<void> {
    std::vector<int> ar;
    ar.reserve(10001);
    for (int i = 0; i < 10000; ++i) ar.push_back((std::max)(a, rand()));
    ar.push_back(a);
    std::sort(ar.begin(), ar.end());
    conn.response_msg(ar[0]);
    co_return;
  }(a, std::move(conn))
                                                .start([](auto &&e) {
                                                });
  return;
}

inline std::string download_10KB(int a) { return std::string(10000, 'A'); }

inline void long_tail_heavy_calculate(coro_rpc::context<int> conn, int a) {
  static std::atomic<uint64_t> g_index = 0;
  g_index++;
  if (g_index % 100 == 0) {
    heavy_calculate(std::move(conn), a);
  }
  else {
    conn.response_msg(a);
  }
}

inline void async_io(coro_rpc::context<int> conn, int a) {
  using namespace std::chrono;
  [executor = pool.get_executor()](
      int a, coro_rpc::context<int> conn) -> async_simple::coro::Lazy<void> {
    auto timer = asio_util::period_timer(executor);
    timer.expires_after(10ms);
    co_await timer.async_await();
    conn.response_msg(a);
  }(a, std::move(conn))
                                                 .start([](auto &&e) {
                                                 });
  return;
}

inline void long_tail_async_io(coro_rpc::context<int> conn, int a) {
  static std::atomic<uint64_t> g_index = 0;
  g_index++;
  if (g_index % 100 == 0) {
    async_io(std::move(conn), a);
  }
  else {
    conn.response_msg(a);
  }
  return;
}

inline void block_io(coro_rpc::context<int> conn, int a) {
  using namespace std::chrono;
  asio::post(pool.get_executor(), [conn = std::move(conn), a]() mutable {
    std::this_thread::sleep_for(50ms);
    conn.response_msg(a);
  });
  return;
}

inline void long_tail_block_io(coro_rpc::context<int> conn, int a) {
  static std::atomic<uint64_t> g_index = 0;
  g_index++;
  if (g_index % 100 == 0) {
    block_io(std::move(conn), a);
    return;
  }
  else {
    conn.response_msg(a);
    return;
  }
}
