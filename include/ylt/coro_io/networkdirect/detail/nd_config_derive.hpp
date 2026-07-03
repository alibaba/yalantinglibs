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

#include <algorithm>

#include "ylt/coro_io/networkdirect/nd_types.hpp"
#include "nd_impl_types.hpp"

namespace coro_io::detail {

inline constexpr size_type default_cqe = 4096;
inline constexpr size_type default_cq_poll_batch = 16;
inline constexpr size_type default_max_send_wr = 128;
inline constexpr size_type default_max_recv_wr = 128;
inline constexpr size_type default_max_send_sge = 4;
inline constexpr size_type default_max_recv_sge = 4;

inline nd_config_t derive_effective_config(nd_config_t const& user_config,
                                           native_context_config_t const& caps) {
  nd_config_t effective = user_config;

  if (effective.cqe_ == 0) {
    effective.cqe_ = (std::min)(caps.MaxCompletionQueueDepth, default_cqe);
  }
  if (effective.cq_poll_batch_ == 0) {
    effective.cq_poll_batch_ =
        static_cast<std::uint32_t>(default_cq_poll_batch);
  }
  if (effective.max_send_wr_ == 0) {
    effective.max_send_wr_ =
        (std::min)(caps.MaxInitiatorQueueDepth, default_max_send_wr);
  }
  if (effective.max_recv_wr_ == 0) {
    effective.max_recv_wr_ =
        (std::min)(caps.MaxReceiveQueueDepth, default_max_recv_wr);
  }
  if (effective.max_send_sge_ == 0) {
    effective.max_send_sge_ =
        (std::min)(caps.MaxInitiatorSge, default_max_send_sge);
  }
  if (effective.max_recv_sge_ == 0) {
    effective.max_recv_sge_ =
        (std::min)(caps.MaxReceiveSge, default_max_recv_sge);
  }
  if (effective.max_inline_data_ == 0) {
    effective.max_inline_data_ = caps.MaxInlineDataSize;
  }
  if (effective.inbound_read_limit_ == 0) {
    effective.inbound_read_limit_ = caps.MaxInboundReadLimit;
  }
  if (effective.outbound_read_limit_ == 0) {
    effective.outbound_read_limit_ = caps.MaxOutboundReadLimit;
  }

  return effective;
}

}  // namespace coro_io::detail
