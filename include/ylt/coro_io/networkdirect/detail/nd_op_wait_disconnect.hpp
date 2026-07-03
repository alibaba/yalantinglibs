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

// Disconnect-notification watcher (on_disconnect). Issues IND2Connector::
// NotifyDisconnect; when that overlapped completes (connection disconnected),
// it sets the connector's peer_closed_ latch and upcalls handler with
// rdma_errc::disconnected. Mirrors ibv_wait_disconnect_op.
template <typename Handler, typename IoExecutor>
class nd_wait_disconnect_op final : public nd_op_base {
private:
  std::atomic<bool>* peer_closed_;
  Handler handler_;
  asio::detail::handler_work<Handler, IoExecutor> work_;

public:
  ASIO_DEFINE_HANDLER_PTR(nd_wait_disconnect_op);
  nd_wait_disconnect_op(IND2Connector* connector,
                        std::atomic<bool>* peer_closed,
                        Handler& handler, const IoExecutor& io_ex)
      : nd_op_base(connector, &nd_op_base::default_process,
                   &nd_wait_disconnect_op::do_complete)
      , peer_closed_(peer_closed)
      , handler_(ASIO_MOVE_CAST(Handler)(handler))
      , work_(handler_, io_ex) {
  }

private:
  static void do_complete(void* owner, asio::detail::operation* base,
                          const asio::error_code& result_ec,
                          std::size_t /*bytes_transferred*/) {
    nd_wait_disconnect_op* o = static_cast<nd_wait_disconnect_op*>(base);

    bool const already_closed =
        o->peer_closed_ &&
        o->peer_closed_->load(std::memory_order_acquire);
    asio::error_code ec = result_ec;
    if (!ec) {
      if (o->peer_closed_) {
        o->peer_closed_->store(true, std::memory_order_release);
      }
      ec = make_error_code(rdma_errc::disconnected);
    } else if (ec == asio::error::operation_aborted && !already_closed) {
      // Per-operation cancellation of NotifyDisconnect must not make the
      // connection appear closed. A self/peer disconnect sets peer_closed_ via
      // disconnect() or a real notification and is reported below.
    } else if (ec == asio::error::operation_aborted && already_closed) {
      if (o->peer_closed_) {
        o->peer_closed_->store(true, std::memory_order_release);
      }
      ec = make_error_code(rdma_errc::disconnected);
    }

    ptr p = {asio::detail::addressof(o->handler_), o, o};

    ASIO_HANDLER_COMPLETION((*o));

    asio::detail::handler_work<Handler, IoExecutor> w(ASIO_MOVE_CAST2(
        asio::detail::handler_work<Handler, IoExecutor>)(o->work_));

    ASIO_ERROR_LOCATION(ec);

    asio::detail::binder1<Handler, asio::error_code> handler(o->handler_, ec);
    p.h = asio::detail::addressof(handler.handler_);
    p.reset();

    if (owner) {
      asio::detail::fenced_block b(asio::detail::fenced_block::half);
      ASIO_HANDLER_INVOCATION_BEGIN((handler.arg1_));
      w.complete(handler, handler.handler_);
      ASIO_HANDLER_INVOCATION_END;
    }
  }
};

}  // namespace coro_io::detail
