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

#include <concepts>
#include <type_traits>

#include "asio/detail/config.hpp"
#include "asio/detail/handler_alloc_helpers.hpp"
#include "asio/detail/memory.hpp"
#include "nd_op_base.hpp"
#include "nd_op_complete.hpp"
#include "nd_op_read.hpp"
#include "nd_op_recv.hpp"
#include "nd_op_send.hpp"
#include "nd_op_write.hpp"
#include "nd_ops_verbs.hpp"
#include "nd_service_base.hpp"
#include "ylt/coro_io/networkdirect/nd_buffer.hpp"
#include "ylt/coro_io/networkdirect/nd_completion_queue.hpp"

namespace coro_io::detail {

// Data-plane service backing nd_queue_pair. The per-QP state
// (implementation_type) is owned by the queue_pair (no io_object_impl); this
// service supplies the verbs logic in two flavors:
//   - static methods (poll mode): operate purely on implementation_type, touch
//   no
//     io_context. Immediate completions are queued on the bound
//     nd_completion_queue.
//   - member methods (event mode): arm the shared-CQ (IOCP) notify and route
//     immediate completions to the scheduler.
//
// Unlike ibv, nd creates and OWNS the QP at construction (impl.qp_ is a
// nd2_queue_pair_ptr); the connector borrows it via native_handle().
class nd_verbs_service
    : public asio::detail::execution_context_service_base<nd_verbs_service>,
      public nd_service_base {
 public:
  using base_type =
      asio::detail::execution_context_service_base<nd_verbs_service>;

  // Owned by the queue_pair. event mode: poll_cq_ == nullptr; poll mode:
  // non-null.
  struct implementation_type {
    nd2_queue_pair_ptr qp_;      // owned (created at construction)
    native_cq_t* cq_ = nullptr;  // CQ the QP is bound to
    nd_adapter_ptr device_;      // for create_qp (pd)
    nd_config_t config_;         // effective config
    nd_completion_queue* poll_cq_ =
        nullptr;  // poll-mode immediate-completion sink
  };

  explicit nd_verbs_service(asio::execution_context& ctx)
      : base_type(ctx), nd_service_base(ctx), success_ec_() {}

  ~nd_verbs_service() = default;

  // The native QP is owned by the queue_pair's impl_; nothing to tear down
  // here.
  void shutdown() override {}

  // Create the QP from the impl's device/cq/config. Static: no io_context
  // needed.
  inline static asio::error_code create_qp(implementation_type& impl);

  // "bound" = associated with a completion mechanism (a CQ). On nd the QP is
  // created at bind time, so native_handle() is also non-null then; the
  // predicate is named for the binding, to match ibv.
  static bool is_bound(implementation_type const& impl) noexcept {
    return impl.cq_ != nullptr;
  }

  static native_qp_t* native_handle(implementation_type const& impl) noexcept {
    return impl.qp_.Get();
  }

  // ---- poll mode: static async ops (no io_context) ----

  template <typename BufferSequence, typename Handler, typename IoExecutor>
  static void async_send_static(implementation_type& impl,
                                BufferSequence const& buffers, Handler& handler,
                                IoExecutor const& io_ex) {
    using op = nd_send_op<BufferSequence, Handler, IoExecutor>;
    typename op::ptr p = {asio::detail::addressof(handler),
                          op::ptr::allocate(handler), 0};
    p.p = new (p.v) op{asio::error_code{}, buffers, handler, io_ex};
    finish_poll(impl, p.p, do_post_send(impl, p.p));
    p.v = p.p = 0;
  }

  template <typename BufferSequence, typename Handler, typename IoExecutor>
  static void async_recv_static(implementation_type& impl,
                                BufferSequence const& buffers, Handler& handler,
                                IoExecutor const& io_ex) {
    using op = nd_recv_op<BufferSequence, Handler, IoExecutor>;
    typename op::ptr p = {asio::detail::addressof(handler),
                          op::ptr::allocate(handler), 0};
    p.p = new (p.v) op{asio::error_code{}, buffers, handler, io_ex};
    finish_poll(impl, p.p, do_post_recv(impl, p.p));
    p.v = p.p = 0;
  }

  template <typename BufferSequence, typename Handler, typename IoExecutor>
  static void async_read_static(implementation_type& impl,
                                BufferSequence const& buffers,
                                nd_remote_addr_t const& remote_addr,
                                Handler& handler, IoExecutor const& io_ex) {
    using op = nd_read_op<BufferSequence, Handler, IoExecutor>;
    typename op::ptr p = {asio::detail::addressof(handler),
                          op::ptr::allocate(handler), 0};
    p.p =
        new (p.v) op{asio::error_code{}, buffers, remote_addr, handler, io_ex};
    finish_poll(impl, p.p, do_post_read(impl, p.p));
    p.v = p.p = 0;
  }

  template <typename BufferSequence, typename Handler, typename IoExecutor>
  static void async_write_static(implementation_type& impl,
                                 BufferSequence const& buffers,
                                 nd_remote_addr_t const& remote_addr,
                                 Handler& handler, IoExecutor const& io_ex) {
    using op = nd_write_op<BufferSequence, Handler, IoExecutor>;
    typename op::ptr p = {asio::detail::addressof(handler),
                          op::ptr::allocate(handler), 0};
    p.p =
        new (p.v) op{asio::error_code{}, buffers, remote_addr, handler, io_ex};
    finish_poll(impl, p.p, do_post_write(impl, p.p));
    p.v = p.p = 0;
  }

  // ---- event mode: member async ops (use the io_context) ----

  template <typename BufferSequence, typename Handler, typename IoExecutor>
  void async_send(implementation_type& impl, BufferSequence const& buffers,
                  Handler& handler, IoExecutor const& io_ex) {
    using op = nd_send_op<BufferSequence, Handler, IoExecutor>;
    typename op::ptr p = {asio::detail::addressof(handler),
                          op::ptr::allocate(handler), 0};
    p.p = new (p.v) op{success_ec_, buffers, handler, io_ex};
    finish_event(impl, p.p, do_post_send(impl, p.p));
    p.v = p.p = 0;
  }

  template <typename BufferSequence, typename Handler, typename IoExecutor>
  void async_recv(implementation_type& impl, BufferSequence const& buffers,
                  Handler& handler, IoExecutor const& io_ex) {
    using op = nd_recv_op<BufferSequence, Handler, IoExecutor>;
    typename op::ptr p = {asio::detail::addressof(handler),
                          op::ptr::allocate(handler), 0};
    p.p = new (p.v) op{success_ec_, buffers, handler, io_ex};
    finish_event(impl, p.p, do_post_recv(impl, p.p));
    p.v = p.p = 0;
  }

  template <typename BufferSequence, typename Handler, typename IoExecutor>
  void async_read(implementation_type& impl, BufferSequence const& buffers,
                  nd_remote_addr_t const& remote_addr, Handler& handler,
                  IoExecutor const& io_ex) {
    using op = nd_read_op<BufferSequence, Handler, IoExecutor>;
    typename op::ptr p = {asio::detail::addressof(handler),
                          op::ptr::allocate(handler), 0};
    p.p = new (p.v) op{success_ec_, buffers, remote_addr, handler, io_ex};
    finish_event(impl, p.p, do_post_read(impl, p.p));
    p.v = p.p = 0;
  }

  template <typename BufferSequence, typename Handler, typename IoExecutor>
  void async_write(implementation_type& impl, BufferSequence const& buffers,
                   nd_remote_addr_t const& remote_addr, Handler& handler,
                   IoExecutor const& io_ex) {
    using op = nd_write_op<BufferSequence, Handler, IoExecutor>;
    typename op::ptr p = {asio::detail::addressof(handler),
                          op::ptr::allocate(handler), 0};
    p.p = new (p.v) op{success_ec_, buffers, remote_addr, handler, io_ex};
    finish_event(impl, p.p, do_post_write(impl, p.p));
    p.v = p.p = 0;
  }

 private:
  asio::error_code success_ec_;

  // Reject a buffer sequence whose SGE count exceeds the device's max_sge
  // before posting -- a clean library error instead of a raw HW failure.
  // Returns true (and sets ec) when over the limit. (sge_max == 0 means the
  // effective config was never derived; treat as "no limit".)
  static bool exceeds_sge_limit(std::size_t sge_count, std::uint32_t sge_max,
                                asio::error_code& ec) {
    if (sge_max != 0 && sge_count > sge_max) {
      ec = make_error_code(rdma_errc::too_many_sge);
      return true;
    }
    return false;
  }

  template <typename BufferSequence>
  static constexpr bool is_single_buffer_sequence_v =
      std::same_as<std::remove_cvref_t<BufferSequence>, const_buffer> ||
      std::same_as<std::remove_cvref_t<BufferSequence>, mutable_buffer>;

  // Post the WR. Returns true if it completed immediately (empty buffers / sync
  // post error) and needs the immediate-completion sink; false if posted (a CQE
  // will complete it).
  template <typename SendOpType>
  static bool do_post_send(implementation_type& impl, SendOpType* op) {
    auto const& buffers = op->get_buffer_sequence();
    if constexpr (is_single_buffer_sequence_v<decltype(buffers)>) {
      if (buffers.length() == 0) {
        return true;
      }
      if (exceeds_sge_limit(1, impl.config_.max_send_sge_, op->ec_)) {
        return true;
      }
      native_sge_t sge{};
      fill_native_sge(sge, buffers);
      verbs_ops::post_send(impl.qp_.Get(), op, &sge, 1, 0, op->ec_);
      return static_cast<bool>(op->ec_);
    }

    nd_sglist_t sglist;
    auto built =
        build_native_sglist(buffers, sglist, impl.config_.max_send_sge_);
    if (built.all_empty) {
      return true;
    }
    if (built.too_many_sge) {
      op->ec_ = make_error_code(rdma_errc::too_many_sge);
      return true;
    }
    verbs_ops::post_send(impl.qp_.Get(), op, built.data, built.count, 0,
                         op->ec_);
    return static_cast<bool>(op->ec_);
  }

  template <typename RecvOpType>
  static bool do_post_recv(implementation_type& impl, RecvOpType* op) {
    auto const& buffers = op->get_buffer_sequence();
    if constexpr (is_single_buffer_sequence_v<decltype(buffers)>) {
      if (buffers.length() == 0) {
        return true;
      }
      if (exceeds_sge_limit(1, impl.config_.max_recv_sge_, op->ec_)) {
        return true;
      }
      native_sge_t sge{};
      fill_native_sge(sge, buffers);
      verbs_ops::post_recv(impl.qp_.Get(), op, &sge, 1, op->ec_);
      return static_cast<bool>(op->ec_);
    }

    nd_sglist_t sglist;
    auto built =
        build_native_sglist(buffers, sglist, impl.config_.max_recv_sge_);
    if (built.all_empty) {
      return true;
    }
    if (built.too_many_sge) {
      op->ec_ = make_error_code(rdma_errc::too_many_sge);
      return true;
    }
    verbs_ops::post_recv(impl.qp_.Get(), op, built.data, built.count, op->ec_);
    return static_cast<bool>(op->ec_);
  }

  template <typename ReadOpType>
  static bool do_post_read(implementation_type& impl, ReadOpType* op) {
    auto const& buffers = op->get_buffer_sequence();
    if constexpr (is_single_buffer_sequence_v<decltype(buffers)>) {
      if (buffers.length() == 0) {
        return true;
      }
      if (exceeds_sge_limit(1, impl.config_.max_send_sge_, op->ec_)) {
        return true;
      }
      native_sge_t sge{};
      fill_native_sge(sge, buffers);
      auto const& ra = op->get_remote_addr();
      verbs_ops::post_read(impl.qp_.Get(), op, &sge, 1, ra.addr_, ra.token_, 0,
                           op->ec_);
      return static_cast<bool>(op->ec_);
    }

    nd_sglist_t sglist;
    auto built =
        build_native_sglist(buffers, sglist, impl.config_.max_send_sge_);
    if (built.all_empty) {
      return true;
    }
    if (built.too_many_sge) {
      op->ec_ = make_error_code(rdma_errc::too_many_sge);
      return true;
    }
    auto const& ra = op->get_remote_addr();
    verbs_ops::post_read(impl.qp_.Get(), op, built.data, built.count, ra.addr_,
                         ra.token_, 0, op->ec_);
    return static_cast<bool>(op->ec_);
  }

  template <typename WriteOpType>
  static bool do_post_write(implementation_type& impl, WriteOpType* op) {
    auto const& buffers = op->get_buffer_sequence();
    if constexpr (is_single_buffer_sequence_v<decltype(buffers)>) {
      if (buffers.length() == 0) {
        return true;
      }
      if (exceeds_sge_limit(1, impl.config_.max_send_sge_, op->ec_)) {
        return true;
      }
      native_sge_t sge{};
      fill_native_sge(sge, buffers);
      auto const& ra = op->get_remote_addr();
      verbs_ops::post_write(impl.qp_.Get(), op, &sge, 1, ra.addr_, ra.token_, 0,
                            op->ec_);
      return static_cast<bool>(op->ec_);
    }

    nd_sglist_t sglist;
    auto built =
        build_native_sglist(buffers, sglist, impl.config_.max_send_sge_);
    if (built.all_empty) {
      return true;
    }
    if (built.too_many_sge) {
      op->ec_ = make_error_code(rdma_errc::too_many_sge);
      return true;
    }
    auto const& ra = op->get_remote_addr();
    verbs_ops::post_write(impl.qp_.Get(), op, built.data, built.count, ra.addr_,
                          ra.token_, 0, op->ec_);
    return static_cast<bool>(op->ec_);
  }

  // poll mode: immediate completions go to the CQ's ready queue (drained by
  // poll()); posted ops complete when the user reaps their CQE.
  static void finish_poll(implementation_type& impl, nd_verbs_op_base* op,
                          bool immediate) {
    if (immediate) {
      impl.poll_cq_->push_ready(op);
    }
  }

  // event mode: a posted op needs nothing here -- the io_completion_service's
  // poller is already armed (started at queue_pair::bind) and
  // self-perpetuating, so it reaps this op's completion. Only an immediate
  // completion (empty buffers / synchronous post error) needs scheduling onto
  // the io_context.
  void finish_event(implementation_type& /*impl*/, nd_verbs_op_base* op,
                    bool immediate) {
    if (immediate) [[unlikely]] {
      nd_complete_op::Handler handler{};
      nd_complete_op::ptr p = {asio::detail::addressof(handler),
                               nd_complete_op::ptr::allocate(handler), 0};
      p.p = new (p.v) nd_complete_op{op};
      this->scheduler_.post_immediate_completion(p.p, false);
      p.v = p.p = 0;
    }
  }
};

// --- inlined implementation ---

// Create the QP from the impl's device/cq/config. Static: no io_context needed.
inline asio::error_code nd_verbs_service::create_qp(implementation_type& impl) {
  asio::error_code ec;
  if (impl.qp_) {
    ec = asio::error::already_open;
    ASIO_ERROR_LOCATION(ec);
    return ec;
  }
  if (!impl.device_ || !impl.device_->pd_ || !impl.cq_) {
    ec = rdma_errc::invalid_device;
    ASIO_ERROR_LOCATION(ec);
    return ec;
  }
  auto const& eff = impl.config_;
  native_qp_init_attr qp_init_attr{
      .qp_context_ = nullptr,
      .rcq_ = impl.cq_,
      .icq_ = impl.cq_,
      .max_send_wr_ = eff.max_send_wr_,
      .max_recv_wr_ = eff.max_recv_wr_,
      .max_send_sge_ = eff.max_send_sge_,
      .max_recv_sge_ = eff.max_recv_sge_,
      .max_inline_data_ = eff.max_inline_data_,
  };
  impl.qp_.Attach(
      verbs_ops::create_qp(impl.device_->pd_.get(), qp_init_attr, ec));
  return ec;
}

}  // namespace coro_io::detail
