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

#include <atomic>

#include "asio/detail/bind_handler.hpp"
#include "asio/detail/fenced_block.hpp"
#include "asio/detail/handler_alloc_helpers.hpp"
#include "asio/detail/handler_work.hpp"
#include "asio/detail/memory.hpp"
#include "nd_op_base.hpp"

namespace coro_io::detail {

// Server-side accept op: completes when IND2Connector::Accept's overlapped
// finishes. On success: CAS connecting -> connected. On operation_aborted
// (per-op cancel or disconnect() race): mark closed (terminal).
template <typename Handler, typename IoExecutor>
class nd_accept_op final : public nd_op_base {
private:
  std::atomic<connect_state>* state_;
  Handler handler_;
  asio::detail::handler_work<Handler, IoExecutor> work_;

public:
  ASIO_DEFINE_HANDLER_PTR(nd_accept_op);
  nd_accept_op(IND2Connector* connector, std::atomic<connect_state>* state,
               Handler& handler, const IoExecutor& io_ex)
      : nd_op_base(connector, &nd_op_base::default_process,
                   &nd_accept_op::do_complete)
      , state_(state)
      , handler_(ASIO_MOVE_CAST(Handler)(handler))
      , work_(handler_, io_ex) {}

private:
  static void do_complete(void* owner, asio::detail::operation* base,
                          const asio::error_code& result_ec,
                          std::size_t /*bytes_transferred*/) {
   asio::error_code ec = result_ec;

   nd_accept_op* o = static_cast<nd_accept_op*>(base);

   if (owner && o->state_) {
     if (!ec) {
       connect_state expected = connect_state::connecting;
       if (!o->state_->compare_exchange_strong(
               expected, connect_state::connected,
               std::memory_order_acq_rel, std::memory_order_acquire)) {
         ec = asio::error::operation_aborted;
       }
     }
     else {
       o->state_->store(connect_state::closed, std::memory_order_release);
     }
   }

   ptr p = {asio::detail::addressof(o->handler_), o, o};

   ASIO_HANDLER_COMPLETION((*o));

   // Take ownership of the operation's outstanding work.
   asio::detail::handler_work<Handler, IoExecutor> w(ASIO_MOVE_CAST2(
       asio::detail::handler_work<Handler, IoExecutor>)(o->work_));

   ASIO_ERROR_LOCATION(ec);

   // Make a copy of the handler so that the memory can be deallocated before
   // the upcall is made. Even if we're not about to make an upcall, a
   // sub-object of the handler may be the true owner of the memory associated
   // with the handler. Consequently, a local copy of the handler is required
   // to ensure that any owning sub-object remains valid until after we have
   // deallocated the memory here.
   asio::detail::binder1<Handler, asio::error_code> handler(o->handler_, ec);
   p.h = asio::detail::addressof(handler.handler_);
   p.reset();

   // Make the upcall if required.
   if (owner) {
     asio::detail::fenced_block b(asio::detail::fenced_block::half);
     ASIO_HANDLER_INVOCATION_BEGIN((handler.arg1_));
     w.complete(handler, handler.handler_);
     ASIO_HANDLER_INVOCATION_END;
   }
  }
};

}  // namespace coro_io::detail
