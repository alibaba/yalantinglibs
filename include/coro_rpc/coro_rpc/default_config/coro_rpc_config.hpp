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
#pragma once
#pragma once

#include <chrono>
#include <thread>

#include "coro_rpc/coro_rpc/protocol/coro_rpc_protocol.hpp"

namespace coro_rpc::config {

struct coro_rpc_config_base {
  uint16_t port = 8801;
  unsigned thread_num = std::thread::hardware_concurrency();
  std::chrono::steady_clock::duration conn_timeout_duration =
      std::chrono::seconds{0};
};

struct coro_rpc_default_config : public coro_rpc_config_base {
  using rpc_protocol = coro_rpc::protocol::coro_rpc_protocol;
};

}  // namespace coro_rpc::config
