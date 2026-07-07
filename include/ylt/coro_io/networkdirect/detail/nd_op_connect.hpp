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
#include <atomic>

#include "asio/buffer.hpp"
#include "asio/detail/bind_handler.hpp"
#include "asio/detail/fenced_block.hpp"
#include "asio/detail/handler_alloc_helpers.hpp"
#include "asio/detail/handler_work.hpp"
#include "asio/detail/memory.hpp"
#include "nd_op_base.hpp"

namespace coro_io::detail {

class nd_connect_op_base : public nd_op_base {
protected:
  using nd_op_base::status_t;
  enum class stage_t {
    connecting,
    connected,
    done,
    error,
  };

protected:
  stage_t stage_;
  std::atomic<connect_state>* state_;
  // Caller-owned reply buffer passed to async_connect. The op only stores the
  // lightweight Asio buffer view; the caller owns the underlying memory.
  asio::mutable_buffer reply_;
  std::size_t reply_len_ = 0;

public:
  inline nd_connect_op_base(IND2Connector* connector,
                               std::atomic<connect_state>* state,
                               asio::mutable_buffer reply,
                               func_type complete_func);

protected:
  IND2Connector* get_connector() const {
    return static_cast<IND2Connector*>(this->get_overlapped());
  }

  inline static status_t do_process(void* owner, nd_op_base* base,
                                       asio::error_code& ec);

  inline status_t process_complete_connect(void* owner,
                                              asio::error_code& ec);

  inline bool mark_connected(asio::error_code& ec);

  inline void capture_remote_pd();
};

template <typename Handler, typename IoExecutor>
class nd_connect_op final : public nd_connect_op_base {
public:
  ASIO_DEFINE_HANDLER_PTR(nd_connect_op);
  using nd_connect_op_base::status_t;

private:
  Handler handler_;
  asio::detail::handler_work<Handler, IoExecutor> work_;

public:
  nd_connect_op(IND2Connector* connector, std::atomic<connect_state>* state,
                asio::mutable_buffer reply, Handler& handler,
                const IoExecutor& io_ex)
      : nd_connect_op_base(connector, state, reply,
                           &nd_connect_op::do_complete)
      , handler_(ASIO_MOVE_CAST(Handler)(handler))
      , work_(handler_, io_ex) {}

private:
  static void do_complete(void* owner, asio::detail::operation* base,
                          const asio::error_code& result_ec,
                          std::size_t bytes_transferred) {
   asio::error_code ec = result_ec;
   nd_connect_op* o = static_cast<nd_connect_op*>(base);

   // resume the operation for the pending steps
   auto const complete_status = o->resume_process(owner, ec);
   if (complete_status != status_t::completed) {
     return;
   }

   if (owner && ec && o->state_) {
     o->state_->store(connect_state::closed, std::memory_order_release);
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
   asio::detail::binder2<Handler, asio::error_code, std::size_t> handler(
       o->handler_, ec, o->reply_len_);
   p.h = asio::detail::addressof(handler.handler_);
   p.reset();

   // Make the upcall if required.
   if (owner) {
     asio::detail::fenced_block b(asio::detail::fenced_block::half);
     ASIO_HANDLER_INVOCATION_BEGIN((handler.arg1_, handler.arg2_));
     w.complete(handler, handler.handler_);
     ASIO_HANDLER_INVOCATION_END;
   }
  }
};

// --- inlined from nd_op_connect.ipp ---

inline nd_connect_op_base::nd_connect_op_base(IND2Connector* connector,
                                       std::atomic<connect_state>* state,
                                       asio::mutable_buffer reply,
                                       func_type complete_func)
   : nd_op_base(connector, &nd_connect_op_base::do_process, complete_func)
   , stage_(stage_t::connecting)
   , state_(state)
   , reply_(reply) {
}

inline nd_connect_op_base::status_t nd_connect_op_base::do_process(
    void* owner, nd_op_base* base, asio::error_code& ec) {
  auto* o = static_cast<nd_connect_op_base*>(base);
  switch (o->stage_) {
    case stage_t::connecting:
      return o->process_complete_connect(owner, ec);
    case stage_t::connected:
      if (!o->mark_connected(ec)) {
        return status_t::completed;
      }
      o->stage_ = stage_t::done;
      return status_t::completed;
    default:
      ec = rdma_errc::invalid_handle;
      o->stage_ = stage_t::error;
      return status_t::completed;
  }
}

inline nd_connect_op_base::status_t nd_connect_op_base::process_complete_connect(
    void* owner, asio::error_code& ec) {
  // ND exposes the accept/reject private data after Connect completes and
  // before CompleteConnect is called.
  capture_remote_pd();
  this->reset();
  auto const hr = get_connector()->CompleteConnect(this);
  if (FAILED(hr)) {
    ec = static_cast<nd_errc>(hr);
    if (state_) state_->store(connect_state::closed, std::memory_order_release);
    stage_ = stage_t::error;
    return status_t::completed;
  }

  stage_ = stage_t::connected;
  if (hr == ND_PENDING) {
    return status_t::continuation;
  }

  if (!mark_connected(ec)) {
    return status_t::completed;
  }
  stage_ = stage_t::done;
  return status_t::completed;
}

inline bool nd_connect_op_base::mark_connected(asio::error_code& ec) {
  if (state_) {
    connect_state expected = connect_state::connecting;
    if (!state_->compare_exchange_strong(
            expected, connect_state::connected,
            std::memory_order_acq_rel, std::memory_order_acquire)) {
      ec = asio::error::operation_aborted;
      stage_ = stage_t::error;
      return false;
    }
  }
  return true;
}

inline void nd_connect_op_base::capture_remote_pd() {
  if (!reply_.data() || reply_.size() == 0) {
    return;
  }
  ULONG pd_size = static_cast<ULONG>(reply_.size());
  auto const hr = get_connector()->GetPrivateData(reply_.data(), &pd_size);
  if (SUCCEEDED(hr) || hr == ND_BUFFER_OVERFLOW) {
    reply_len_ = (std::min)(static_cast<std::size_t>(pd_size),
                            reply_.size());
  }
}

}  // namespace coro_io::detail
