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
#include <cstddef>

#include "asio/detail/op_queue.hpp"
#include "asio/error.hpp"
#include "ylt/coro_io/networkdirect/nd_buffer.hpp"
#include "ylt/coro_io/networkdirect/nd_commons.hpp"

namespace coro_io::detail {

class nd_verbs_op_base {
 public:
  friend class asio::detail::op_queue_access;

  using complete_func =
      void (*)(void*, nd_verbs_op_base*, asio::error_code const&, std::size_t);

  enum class op_type {
    post_recv = 0,
    post_send,
    remote_read,
    remote_write,
    max_ops,
  };

 protected:
  complete_func complete_func_;
  nd_verbs_op_base* next_;

 public:
  asio::error_code ec_;
  std::size_t bytes_transferred_;
  void* cancellation_key_;

  nd_verbs_op_base(complete_func complete_cb,
                   asio::error_code const& success_ec)
      : complete_func_(complete_cb),
        next_(nullptr),
        ec_(success_ec),
        bytes_transferred_(0),
        cancellation_key_(nullptr) {
    assert(complete_func_);
  }

  void complete(void* owner) {
    assert(complete_func_);
    complete_func_(owner, this, ec_, bytes_transferred_);
  }

  void destroy() {
    assert(complete_func_);
    complete_func_(nullptr, this, asio::error_code{}, 0);
  }
};

template <typename BufferSequence>
struct nd_two_sided_op;
template <typename BufferSequence>
struct nd_one_sided_op;

template <mr_adapted_buffer_sequence BufferSequence>
class nd_two_sided_op<BufferSequence> : public nd_verbs_op_base {
 public:
  using complete_func = nd_verbs_op_base::complete_func;
  using op_type = nd_verbs_op_base::op_type;

 protected:
  BufferSequence buffer_seq_;

 public:
  nd_two_sided_op(complete_func complete_cb,
                  asio::error_code const& success_ec,
                  BufferSequence const& buffer_seq)
      : nd_verbs_op_base(complete_cb, success_ec), buffer_seq_(buffer_seq) {}

  BufferSequence const& get_buffer_sequence() const noexcept {
    return buffer_seq_;
  }
};

template <typename BufferSequence>
class nd_one_sided_op : public nd_two_sided_op<BufferSequence> {
 public:
  using base_type = nd_two_sided_op<BufferSequence>;
  using complete_func = typename base_type::complete_func;
  using op_type = typename base_type::op_type;

 protected:
  nd_remote_addr_t remote_addr_;

 public:
  nd_one_sided_op(complete_func complete_cb,
                  asio::error_code const& success_ec,
                  BufferSequence const& buffer_seq,
                  nd_remote_addr_t const& remote_addr)
      : base_type(complete_cb, success_ec, buffer_seq),
        remote_addr_(remote_addr) {}

  nd_remote_addr_t const& get_remote_addr() const noexcept {
    return remote_addr_;
  }
};

}  // namespace coro_io::detail
