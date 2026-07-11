/*
 * Copyright (c) 2025, Alibaba Group Holding Limited;
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

#include <cstdint>

namespace coro_io {

enum mr_acccess_flag_t {
  mr_access_local_write,
  mr_access_remote_read,
  mr_access_remote_write,
};

enum class completion_mode {
  none,
  event,
  poll,
};

struct nd_config_t {
  std::uint32_t cqe_ = 0;
  std::uint32_t cq_poll_batch_ = 0;

  std::uint32_t max_send_wr_ = 0;
  std::uint32_t max_recv_wr_ = 0;
  std::uint32_t max_send_sge_ = 0;
  std::uint32_t max_recv_sge_ = 0;
  std::uint32_t max_inline_data_ = 0;

  std::uint32_t inbound_read_limit_ = 0;
  std::uint32_t outbound_read_limit_ = 0;

  int backlog_ = 128;
};

struct nd_remote_addr_t {
  std::uint64_t addr_;
  std::uint32_t token_;
};

namespace detail {
struct nd_const_buffer_tag {};
struct nd_mutable_buffer_tag {};
}  // namespace detail

}  // namespace coro_io
