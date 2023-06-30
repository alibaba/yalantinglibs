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
#pragma once

#include <chrono>
#include <thread>

#include "ylt/coro_io/io_context_pool.hpp"
#include "ylt/coro_rpc/coro_rpc_server.hpp"
#include "ylt/coro_rpc/impl/context.hpp"
#include "ylt/coro_rpc/impl/protocol/coro_rpc_protocol.hpp"

namespace coro_rpc {

namespace config {
struct coro_rpc_config_base {
  uint16_t port = 8801;
  unsigned thread_num = std::thread::hardware_concurrency();
  std::chrono::steady_clock::duration conn_timeout_duration =
      std::chrono::seconds{0};
};

struct coro_rpc_default_config : public coro_rpc_config_base {
  using rpc_protocol = coro_rpc::protocol::coro_rpc_protocol;
  using executor_pool_t = coro_io::io_context_pool;
};
}  // namespace config

using coro_rpc_server = coro_rpc_server_base<config::coro_rpc_default_config>;
}  // namespace coro_rpc
