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

#include "asio/detail/config.hpp"
#include "asio/detail/io_object_impl.hpp"
#include "asio/detail/object_pool.hpp"
#include "asio/detail/win_iocp_io_context.hpp"
#include "asio/execution_context.hpp"
#include "ylt/coro_io/networkdirect/nd_device.hpp"

namespace coro_io::detail {

// base class of network direct service
class nd_service_base {
 public:
  struct base_implementation_type {
    base_implementation_type* next_;
    base_implementation_type* prev_;
  };

 protected:
  asio::detail::win_iocp_io_context& scheduler_;
  asio::detail::mutex mutex_;
  base_implementation_type* impl_list_;

 protected:
  inline static asio::detail::win_iocp_io_context& use_asio_scheduler(
      asio::execution_context& context);

  inline explicit nd_service_base(asio::execution_context& context);

  inline void base_construct(base_implementation_type& impl);

  inline void base_move_construct(base_implementation_type& impl,
                                  base_implementation_type& other_impl);

  inline void base_destroy(base_implementation_type& impl);

  template <typename ImplType, typename Destroyer>
  void base_shutdown(Destroyer const& destroyer) {
    asio::detail::mutex::scoped_lock lock(mutex_);
    base_implementation_type* impl = impl_list_;
    while (impl) {
      destroyer(static_cast<ImplType&>(*impl));
      impl = impl->next_;
    }
  }

  inline void insert(base_implementation_type& impl);

  inline void remove(base_implementation_type& impl);

  inline void do_insert(base_implementation_type& impl);

  inline void do_remove(base_implementation_type& impl);
};

// --- inlined implementation ---

inline asio::detail::win_iocp_io_context& nd_service_base::use_asio_scheduler(
    asio::execution_context& context) {
  return asio::use_service<asio::detail::win_iocp_io_context>(context);
}

inline nd_service_base::nd_service_base(asio::execution_context& context)
    : scheduler_(use_asio_scheduler(context)), mutex_(), impl_list_(nullptr) {}

inline void nd_service_base::base_construct(base_implementation_type& impl) {
  asio::detail::mutex::scoped_lock lock(mutex_);
  do_insert(impl);
}

inline void nd_service_base::base_move_construct(
    base_implementation_type& impl, base_implementation_type& other_impl) {
  asio::detail::mutex::scoped_lock lock(mutex_);
  do_insert(impl);
}

inline void nd_service_base::base_destroy(base_implementation_type& impl) {
  asio::detail::mutex::scoped_lock lock(mutex_);
  do_remove(impl);
}

inline void nd_service_base::insert(base_implementation_type& impl) {
  asio::detail::mutex::scoped_lock lock(mutex_);
  do_insert(impl);
}

inline void nd_service_base::remove(base_implementation_type& impl) {
  asio::detail::mutex::scoped_lock lock(mutex_);
  do_remove(impl);
}

inline void nd_service_base::do_insert(base_implementation_type& impl) {
  impl.next_ = impl_list_;
  impl.prev_ = nullptr;
  if (impl_list_) {
    impl_list_->prev_ = &impl;
  }
  impl_list_ = &impl;
}

inline void nd_service_base::do_remove(base_implementation_type& impl) {
  if (impl_list_ == &impl) {
    impl_list_ = impl.next_;
  }
  if (impl.prev_) {
    impl.prev_->next_ = impl.next_;
  }
  if (impl.next_) {
    impl.next_->prev_ = impl.prev_;
  }
  impl.next_ = nullptr;
  impl.prev_ = nullptr;
}

}  // namespace coro_io::detail
