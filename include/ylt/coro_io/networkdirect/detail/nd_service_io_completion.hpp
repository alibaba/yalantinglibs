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

#include <array>
#include <atomic>
#include <ranges>
#include <vector>

#include "asio/detail/config.hpp"
#include "asio/detail/handler_alloc_helpers.hpp"
#include "asio/detail/op_queue.hpp"
#include "asio/detail/win_iocp_io_context.hpp"
#include "asio/execution_context.hpp"
#include "nd_config_derive.hpp"
#include "nd_device_impl.hpp"
#include "nd_impl_types.hpp"
#include "nd_op_base.hpp"
#include "nd_ops_verbs.hpp"
#include "nd_verbs_op.hpp"
#include "ylt/coro_io/networkdirect/nd_error.hpp"
#include "ylt/coro_io/networkdirect/nd_types.hpp"

namespace coro_io::detail {

// Per-io_context singleton owning a shared CQ + IOCP overlapped handle. The CQ
// poller is started lazily when the first event-mode queue_pair binds
// (ensure_poller_started()).
//
// Thread-safe & lock-free: the poller is a single self-perpetuating op (a fresh
// IOCP Notify each cycle, but only ONE outstanding at a time), so the CQ
// GetResults is serialized across run() threads with no lock; submitter threads
// only post on their QP and touch no service state.
//
// Consequence (mirrors ibv): once started, the poller is outstanding IOCP work
// for the io_context's lifetime, so io_context::run() no longer returns merely
// because the data plane is idle -- stop via io_context::stop() / destruction.
// poll-mode-only / control-plane-only io_contexts never start it.
class nd_io_completion_service
    : public asio::detail::execution_context_service_base<
          nd_io_completion_service> {
 public:
  using base_type =
      asio::detail::execution_context_service_base<nd_io_completion_service>;

  explicit nd_io_completion_service(asio::execution_context& ctx)
      : base_type(ctx),
        scheduler_(asio::use_service<asio::detail::win_iocp_io_context>(ctx)) {}

  ~nd_io_completion_service() = default;

  inline void shutdown() override;

  // Create the shared CQ + overlapped handle on the IOCP scheduler. The device
  // is used transiently for its adapter (not stored); cqe is the derived CQ
  // depth.
  inline void initialize(nd_adapter_ptr const& device, std::uint32_t cqe,
                         std::uint32_t poll_batch, asio::error_code& ec);

  native_cq_t* get_cq() const noexcept { return cq_.Get(); }

  // Start the self-perpetuating CQ poller. Idempotent + thread-safe: the first
  // event-mode queue_pair to bind fires it; after that it re-arms itself
  // forever.
  inline void ensure_poller_started();

 private:
  // Single-in-flight, self-perpetuating CQ poller. Each cycle uses a fresh IOCP
  // overlapped op (allocated in arm_poller), so only one is ever outstanding --
  // GetResults is serialized. On completion it drains+dispatches the CQ and
  // re-arms; owner == nullptr (io_context shutdown) frees without re-arming.
  class nd_poll_wc_op final : public asio::detail::operation {
   public:
    using base_type = asio::detail::operation;
    struct Handler {};
    ASIO_DEFINE_HANDLER_PTR(nd_poll_wc_op);

    explicit nd_poll_wc_op(nd_io_completion_service* svc)
        : base_type(&nd_poll_wc_op::do_complete), svc_(svc) {}

   private:
    inline static void do_complete(void* owner, base_type* base,
                                   asio::error_code const& result_ec,
                                   std::size_t bytes_transferred);

    nd_io_completion_service* svc_;
  };

  inline static nd_verbs_op_base* resolve_wc(native_wc_t const& result);

  inline void poll_and_dispatch(void* owner);

  // Arm (or re-arm) the single poller: allocate a fresh overlapped op and
  // request an IOCP completion notification for the next CQ event.
  inline void arm_poller();

  asio::detail::win_iocp_io_context& scheduler_;
  nd2_completion_queue_ptr cq_;
  unique_handle_t cq_handle_;
  std::vector<native_wc_t> wc_buf_;
  std::atomic<bool> poller_started_{false};
};

// --- inlined implementation ---

inline void nd_io_completion_service::shutdown() {
  cq_.Reset();
  cq_handle_.reset();
}

// Create the shared CQ + overlapped handle on the IOCP scheduler. The device is
// used transiently for its adapter (not stored); cqe is the derived CQ depth.
inline void nd_io_completion_service::initialize(nd_adapter_ptr const& device,
                                                 std::uint32_t cqe,
                                                 std::uint32_t poll_batch,
                                                 asio::error_code& ec) {
  if (cq_) {
    ec = rdma_errc::already_registered;
    ASIO_ERROR_LOCATION(ec);
    return;
  }
  if (!device || !device->adapter_) {
    ec = rdma_errc::invalid_device;
    ASIO_ERROR_LOCATION(ec);
    return;
  }

  cq_handle_.reset(create_overlapped_file(device->adapter_.Get(), ec));
  if (ec) {
    ASIO_ERROR_LOCATION(ec);
    return;
  }

  native_cq_init_attr cq_init_attr{
      .overlapped_handle_ = cq_handle_.get(),
      .processor_group_ = 0,
      .processor_affinity_ = 0,
  };
  cq_.Attach(
      verbs_ops::create_cq(device->adapter_.Get(), cqe, cq_init_attr, ec));
  if (ec) {
    cq_handle_.reset();
    ASIO_ERROR_LOCATION(ec);
    return;
  }

  scheduler_.register_handle(cq_handle_.get(), ec);
  if (ec) {
    cq_.Reset();
    cq_handle_.reset();
    ASIO_ERROR_LOCATION(ec);
    return;
  }
  wc_buf_.resize(poll_batch ? poll_batch : default_cq_poll_batch);
}

// Start the self-perpetuating CQ poller. Idempotent + thread-safe: the first
// event-mode queue_pair to bind fires it; after that it re-arms itself forever.
inline void nd_io_completion_service::ensure_poller_started() {
  if (!poller_started_.exchange(true, std::memory_order_acq_rel)) {
    arm_poller();
  }
}

inline void nd_io_completion_service::nd_poll_wc_op::do_complete(
    void* owner, base_type* base, asio::error_code const& /*result_ec*/,
    std::size_t /*bytes_transferred*/) {
  auto* o = static_cast<nd_poll_wc_op*>(base);
  nd_io_completion_service* svc = o->svc_;
  ptr p = {nullptr, o, o};
  p.reset();  // free this op (a fresh one is allocated on re-arm)
  if (owner) {
    svc->poll_and_dispatch(owner);
    svc->arm_poller();  // self-perpetuate
  }
}

inline nd_verbs_op_base* nd_io_completion_service::resolve_wc(
    native_wc_t const& result) {
  assert(result.RequestContext);
  auto* op = reinterpret_cast<nd_verbs_op_base*>(result.RequestContext);
  if (result.Status == ND_CANCELED) {
    op->ec_ = asio::error::operation_aborted;
  }
  else {
    op->ec_ = static_cast<nd_errc>(result.Status);
  }
  if (!op->ec_) {
    // send/write byte counts are set on the op at post time.
    if (result.RequestType != ND2_REQUEST_TYPE::Nd2RequestTypeSend &&
        result.RequestType != ND2_REQUEST_TYPE::Nd2RequestTypeWrite) {
      op->bytes_transferred_ = result.BytesTransferred;
    }
  }
  else {
    op->bytes_transferred_ = 0;
  }
  return op;
}

inline void nd_io_completion_service::poll_and_dispatch(void* owner) {
  asio::detail::op_queue<nd_verbs_op_base> ops;
  ULONG n = 0;
  do {
    n = verbs_ops::poll_cq(cq_.Get(), wc_buf_.data(),
                           static_cast<size_type>(wc_buf_.size()));
    std::ranges::for_each_n(wc_buf_.begin(), n, [&](auto const& wc) {
      ops.push(resolve_wc(wc));
    });
  } while (n != 0);
  while (auto* op = ops.front()) {
    ops.pop();
    op->complete(owner);
  }
}

// Arm (or re-arm) the single poller: allocate a fresh overlapped op and request
// an IOCP completion notification for the next CQ event.
inline void nd_io_completion_service::arm_poller() {
  nd_poll_wc_op::Handler handler{};
  nd_poll_wc_op::ptr p = {asio::detail::addressof(handler),
                          nd_poll_wc_op::ptr::allocate(handler), 0};
  p.p = new (p.v) nd_poll_wc_op{this};
  asio::error_code ec;
  native_cq_notify_attr notify_attr{
      .type_ = ND_CQ_NOTIFY_ANY,
      .op_ = p.p,
  };
  auto const hr = verbs_ops::notify_cq(cq_.Get(), notify_attr, ec);
  scheduler_.work_started();
  if (!ec && hr == ND_PENDING) {
    scheduler_.on_pending(p.p);
  }
  else {
    scheduler_.on_completion(p.p, ec, 0L);
  }
  p.v = p.p = nullptr;
}

}  // namespace coro_io::detail
