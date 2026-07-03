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

#include <cassert>

#include "nd_op_base.hpp"

namespace coro_io::detail {

// The nd_verbs_op class is not the sub class of asio::detail::operation,
// aka. the win_iocp_operation class, which is inherited from OVERLAPPED.
// Thus, nd_verbs_op needs a wrapper class, what is inherited from OVERLAPPED,
// to be enqueued into the IOCP completion queue.
// The nd_complete_op actually covers this.
class nd_complete_op final : public asio::detail::operation {
public:
  struct Handler {};
  ASIO_DEFINE_HANDLER_PTR(nd_complete_op);

private:
  nd_verbs_op_base* op_;

public:
  explicit nd_complete_op(nd_verbs_op_base* verbs_op)
      : asio::detail::operation(&nd_complete_op::do_complete)
      , op_(verbs_op){
  }

private:
  inline static void do_complete(void* owner, asio::detail::operation* base_op,
                                    asio::error_code const& ec,
                                    std::size_t bytes_transferred);
};

// --- inlined from nd_op_complete.ipp ---

inline void nd_complete_op::do_complete(void* owner, asio::detail::operation* base_op,
                                 asio::error_code const& /*ec*/,
                                 std::size_t /*bytes_transferred*/) {
  nd_complete_op* o = static_cast<nd_complete_op*>(base_op);
  auto* op = o->op_;
  assert(op);

  // ptr object for destruct and free operation
  ptr p = {nullptr, o, o};
  p.reset();

  // callback
  op->complete(owner);
}

}  // namespace coro_io::detail
