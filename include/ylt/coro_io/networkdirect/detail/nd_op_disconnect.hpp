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

#include "asio/detail/handler_alloc_helpers.hpp"
#include "asio/detail/memory.hpp"
#include "nd_op_base.hpp"

namespace coro_io::detail {

// Fire-and-forget disconnect op (no handler): backs the synchronous,
// non-blocking connector::disconnect(). ND2 Disconnect is overlapped, so we
// issue it with this self-reaping op and return without waiting; when IOCP
// completes the overlapped, do_complete simply frees the op (no user upcall).
// Mirrors nd_poll_wc_op's self-perpetuating/self-reaping pattern.
class nd_disconnect_op final : public nd_op_base {
 public:
  struct Handler {};
  ASIO_DEFINE_HANDLER_PTR(nd_disconnect_op);

  inline explicit nd_disconnect_op(IND2Connector* connector);

 private:
  inline static void do_complete(void* /*owner*/, asio::detail::operation* base,
                                 const asio::error_code& /*result_ec*/,
                                 std::size_t /*bytes_transferred*/);
};

// --- inlined from nd_op_disconnect.ipp ---

inline nd_disconnect_op::nd_disconnect_op(IND2Connector* connector)
    : nd_op_base(connector, &nd_op_base::default_process,
                 &nd_disconnect_op::do_complete) {}

inline void nd_disconnect_op::do_complete(void* /*owner*/,
                                          asio::detail::operation* base,
                                          const asio::error_code& /*result_ec*/,
                                          std::size_t /*bytes_transferred*/) {
  auto* o = static_cast<nd_disconnect_op*>(base);
  ptr p = {nullptr, o, o};
  p.reset();  // fire-and-forget: free the op, no upcall
}

}  // namespace coro_io::detail
