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

#include "asio/detail/operation.hpp"
#include "asio/detail/win_iocp_io_context.hpp"
#include "nd_verbs_op.hpp"
#include "ylt/coro_io/networkdirect/nd_buffer.hpp"
#include "ylt/coro_io/networkdirect/nd_error.hpp"
#include "ylt/coro_io/networkdirect/nd_types.hpp"

namespace coro_io::detail {

class nd_op_base : public asio::detail::operation {
 public:
  enum class status_t {
    completed,
    continuation,
  };
  using process_func = status_t (*)(void* owner, nd_op_base* base,
                                    asio::error_code& ec);

 private:
  IND2Overlapped* overlapped_;
  process_func process_func_;

 protected:
  nd_op_base(IND2Overlapped* overlapped, process_func perform_function,
             func_type complete_func)
      : asio::detail::operation(complete_func),
        overlapped_(overlapped),
        process_func_(perform_function) {}

  IND2Overlapped* get_overlapped() const { return overlapped_; }

  static status_t default_process(void* owner, nd_op_base* base,
                                  asio::error_code& ec) {
    return status_t::completed;
  }

  inline status_t resume_process(void* owner, asio::error_code& ec);

 private:
  inline status_t do_process(void* owner, asio::error_code& ec);
};

// --- inlined from nd_op_base.ipp ---

inline nd_op_base::status_t nd_op_base::resume_process(void* owner,
                                                       asio::error_code& ec) {
  if (ec) {
    return status_t::completed;
  }
  auto const status = do_process(owner, ec);
  if (status_t::continuation == status) {
    assert(owner);
    auto* context = static_cast<asio::detail::win_iocp_io_context*>(owner);
    context->work_started();
    context->on_pending(this);
  }
  return status;
}

inline nd_op_base::status_t nd_op_base::do_process(void* owner,
                                                   asio::error_code& ec) {
  assert(overlapped_);

  if (process_func_) {
    return process_func_(owner, this, ec);
  }
  return status_t::completed;
}

}  // namespace coro_io::detail
