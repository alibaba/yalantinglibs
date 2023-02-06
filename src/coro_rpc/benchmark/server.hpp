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

#include "api/rpc_functions.hpp"
#include "coro_rpc/coro_rpc_server.hpp"

inline int start_server(coro_rpc::coro_rpc_server<>& server) {
  using namespace coro_rpc;
  server.regist_handler<
      echo_4B, echo_100B, echo_500B, echo_1KB, echo_5KB, echo_10KB, async_io,
      block_io, heavy_calculate, long_tail_async_io, long_tail_block_io,
      long_tail_heavy_calculate, array_1K_int, array_1K_str_4B, array_1K_rect,
      monsterFunc, ValidateRequestFunc, download_10KB>();
  server.regist_handler<
      many_argument<int, int, int, int, int, int, int, int, int, int, double,
                    double, double, double, double, double>>();
  std::cout << "hardware_concurrency=" << std::thread::hardware_concurrency()
            << std::endl;
  try {
    std::thread thrd{[] {
      pool.run();
    }};
    [[maybe_unused]] auto ec = server.start();
    assert(ec == std::errc{});
    pool.stop();
    thrd.join();
  } catch (std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
  }
  return 0;
}
