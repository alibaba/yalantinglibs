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
#include <system_error>

#include "asio/associated_cancellation_slot.hpp"
#include "asio/buffer.hpp"
#include "asio/cancellation_type.hpp"
#include "asio/detail/config.hpp"
#include "asio/detail/handler_alloc_helpers.hpp"
#include "asio/detail/memory.hpp"
#include "asio/ip/address.hpp"
#include "nd_config_derive.hpp"
#include "nd_op_accept.hpp"
#include "nd_op_connect.hpp"
#include "nd_op_disconnect.hpp"
#include "nd_op_wait_disconnect.hpp"
#include "nd_ops_cm.hpp"
#include "nd_service_base.hpp"
#include "nd_service_device.hpp"

namespace coro_io::detail {

// Control-plane service for nd_connector. Mirrors ibv_connector_service.
//
// Open creates the IND2Connector + overlapped handle (no QP).
// The QP is supplied at async_connect/async_accept time and only borrowed --
// the queue_pair owns it.
template <typename PortSpace>
class nd_connector_service
    : public asio::detail::execution_context_service_base<
          nd_connector_service<PortSpace>>,
      public nd_service_base {
 public:
  using base_type = asio::detail::execution_context_service_base<
      nd_connector_service<PortSpace>>;
  using endpoint_type = typename PortSpace::endpoint;
  using native_connector_type = nd_connector_handle_t;

  struct implementation_type : nd_service_base::base_implementation_type {
    nd2_connector_ptr connector_;
    unique_handle_t connector_handle_;
    nd_adapter_ptr adapter_;
    // Teardown/lifecycle state machine (aligned with ibv's connect_state_):
    // idle -> connecting -> connected -> closed. Atomic: disconnect() is
    // MT-safe (callable from any thread) via CAS arbitration.
    std::atomic<connect_state> connect_state_{connect_state::idle};
    // Peer-disconnect notification latch (set by self disconnect() or
    // NotifyDisconnect). SEPARATE from connect_state_ (mirrors ibv's
    // peer_closed_): makes async_wait_disconnect level-triggered.
    std::atomic<bool> peer_closed_{false};
  };

  explicit nd_connector_service(asio::execution_context& ctx)
      : base_type(ctx),
        nd_service_base(ctx),
        device_svc_(asio::use_service<nd_device_service>(ctx)) {}

  ~nd_connector_service() = default;

  void shutdown() override {
    base_shutdown<implementation_type>([](implementation_type& impl) {
      impl.connector_.Reset();
      impl.connector_handle_.reset();
      impl.adapter_.reset();
    });
  }

  // lifecycle
  void construct(implementation_type& impl) {
    nd_service_base::base_construct(impl);
    impl.connector_.Reset();
    impl.connector_handle_.reset();
    impl.adapter_.reset();
    impl.connect_state_.store(connect_state::idle, std::memory_order_relaxed);
    impl.peer_closed_.store(false, std::memory_order_relaxed);
  }

  void destroy(implementation_type& impl) {
    impl.connector_.Reset();
    impl.connector_handle_.reset();
    impl.adapter_.reset();
    nd_service_base::base_destroy(impl);
  }

  void move_construct(implementation_type& impl,
                      implementation_type& other_impl) {
    nd_service_base::base_move_construct(impl, other_impl);
    impl.connector_ = std::move(other_impl.connector_);
    impl.connector_handle_ = std::move(other_impl.connector_handle_);
    impl.adapter_ = std::move(other_impl.adapter_);
    impl.connect_state_.store(
        other_impl.connect_state_.load(std::memory_order_relaxed),
        std::memory_order_relaxed);
    impl.peer_closed_.store(
        other_impl.peer_closed_.load(std::memory_order_relaxed),
        std::memory_order_relaxed);
  }

  void move_assign(implementation_type& impl,
                   nd_connector_service& other_service,
                   implementation_type& other_impl) {
    impl.connector_.Reset();
    impl.connector_handle_.reset();
    nd_service_base::base_destroy(impl);
    nd_service_base::base_construct(impl);
    impl.connector_ = std::move(other_impl.connector_);
    impl.connector_handle_ = std::move(other_impl.connector_handle_);
    impl.adapter_ = std::move(other_impl.adapter_);
    impl.connect_state_.store(
        other_impl.connect_state_.load(std::memory_order_relaxed),
        std::memory_order_relaxed);
    impl.peer_closed_.store(
        other_impl.peer_closed_.load(std::memory_order_relaxed),
        std::memory_order_relaxed);
  }

  // open (client): create IND2Connector + overlapped handle. PortSpace value is
  // accepted for parity with ibv; nd has no rdma port space, so it's ignored.
  // Requires use_device() on this io_context (config is centralized there).
  void open(implementation_type& impl, PortSpace const& /*ps*/,
            asio::error_code& ec) {
    do_open(impl, ec);
  }

  // assign (server): adopt a connector handle from the listener. The client's
  // request private data is delivered separately, into the caller's buffer at
  // async_get_connection (see nd_listener), so it is not stored here. Mirrors
  // ibv.
  void assign(implementation_type& impl, native_connector_type&& handle,
              asio::error_code& ec) {
    if (impl.connector_) {
      ec = asio::error::already_open;
      ASIO_ERROR_LOCATION(ec);
      return;
    }
    if (!handle.connector_) {
      ec = rdma_errc::invalid_handle;
      ASIO_ERROR_LOCATION(ec);
      return;
    }

    this->scheduler_.register_handle(handle.overlapped_handle_.get(), ec);
    if (ec) {
      ASIO_ERROR_LOCATION(ec);
      return;
    }

    impl.connector_ = std::move(handle.connector_);
    impl.connector_handle_ = std::move(handle.overlapped_handle_);
    impl.adapter_ = std::move(handle.adapter_);
    impl.connect_state_.store(connect_state::idle, std::memory_order_relaxed);
    impl.peer_closed_.store(false, std::memory_order_relaxed);
  }

  bool is_open(implementation_type const& impl) const noexcept {
    return impl.connector_ != nullptr;
  }

  // cancel() is intentionally removed from the connector -- disconnect() is the
  // unified teardown mechanism (mirrors ibv). Per-op cancellation is handled
  // via cancellation_slot (CancelIoEx on the individual OVERLAPPED).

  // async connect: borrow qp from the queue_pair, auto-open if needed, then
  // Bind + Connect. CompleteConnect captures the server's reply private data.
  template <typename Handler, typename IoExecutor>
  void async_connect(implementation_type& impl, native_qp_t* qp,
                     endpoint_type const& endpoint, asio::const_buffer request,
                     asio::mutable_buffer reply, Handler& handler,
                     IoExecutor const& io_ex) {
    auto cancel_slot = asio::get_associated_cancellation_slot(handler);
    // Auto-open (mirrors asio socket.connect opening with the protocol).
    asio::error_code open_ec;
    if (!is_open(impl)) {
      do_open(impl, open_ec);
    }

    using op = nd_connect_op<Handler, IoExecutor>;
    typename op::ptr p = {asio::detail::addressof(handler),
                          op::ptr::allocate(handler), 0};
    // reply is the caller's mutable buffer, filled with the server's reply pd
    // on CompleteConnect. The request (below) is copied by ND2 Connect
    // synchronously.
    p.p = new (p.v)
        op{impl.connector_.Get(), &impl.connect_state_, reply, handler, io_ex};

    if (open_ec) {
      this->scheduler_.work_started();
      this->scheduler_.on_completion(p.p, open_ec);
      p.v = p.p = 0;
      return;
    }
    if (request.size() > max_outgoing_private_data) {
      asio::error_code ec = rdma_errc::private_data_too_large;
      this->scheduler_.work_started();
      this->scheduler_.on_completion(p.p, ec);
      p.v = p.p = 0;
      return;
    }
    // Terminal / non-fresh connector: a prior connect attempt (one-shot) or a
    // disconnect()/failure left it non-idle. Early-exit with a clear code
    // instead of reusing a stranded IND2Connector; the user must create a fresh
    // connector. Mirrors the ibv `connect_state_ != idle` guard. [Windows
    // verify]
    if (impl.connect_state_.load(std::memory_order_acquire) !=
        connect_state::idle) {
      asio::error_code ec = rdma_errc::connector_terminal;
      this->scheduler_.work_started();
      this->scheduler_.on_completion(p.p, ec);
      p.v = p.p = 0;
      return;
    }
    if (!qp) {
      asio::error_code ec = rdma_errc::invalid_handle;
      this->scheduler_.work_started();
      this->scheduler_.on_completion(p.p, ec);
      p.v = p.p = 0;
      return;
    }
    start_connect_op(impl, qp, endpoint, request, p.p);
    arm_nd_cancellation(cancel_slot, impl.connector_handle_.get(), p.p);
    p.v = p.p = 0;
  }

  // async accept: borrow qp from the queue_pair, then Accept.
  template <typename Handler, typename IoExecutor>
  void async_accept(implementation_type& impl, native_qp_t* qp,
                    asio::const_buffer private_data, Handler& handler,
                    IoExecutor const& io_ex) {
    auto cancel_slot = asio::get_associated_cancellation_slot(handler);
    using op = nd_accept_op<Handler, IoExecutor>;
    typename op::ptr p = {asio::detail::addressof(handler),
                          op::ptr::allocate(handler), 0};
    p.p = new (p.v)
        op{impl.connector_.Get(), &impl.connect_state_, handler, io_ex};
    if (!device_registered()) {
      asio::error_code ec = rdma_errc::device_not_registered;
      this->scheduler_.work_started();
      this->scheduler_.on_completion(p.p, ec);
      p.v = p.p = 0;
      return;
    }
    if (private_data.size() > max_outgoing_private_data) {
      asio::error_code ec = rdma_errc::private_data_too_large;
      this->scheduler_.work_started();
      this->scheduler_.on_completion(p.p, ec);
      p.v = p.p = 0;
      return;
    }
    if (impl.connect_state_.load(std::memory_order_acquire) !=
        connect_state::idle) {
      asio::error_code ec = rdma_errc::connector_terminal;
      this->scheduler_.work_started();
      this->scheduler_.on_completion(p.p, ec);
      p.v = p.p = 0;
      return;
    }
    if (!qp) {
      asio::error_code ec = rdma_errc::invalid_handle;
      this->scheduler_.work_started();
      this->scheduler_.on_completion(p.p, ec);
      p.v = p.p = 0;
      return;
    }
    start_accept_op(impl, qp, private_data, p.p);
    arm_nd_cancellation(cancel_slot, impl.connector_handle_.get(), p.p);
    p.v = p.p = 0;
  }

  // MT-safe disconnect (mirrors ibv CAS pattern). Callable from any thread.
  // Atomically transitions connect_state_ to closed, then dispatches based on
  // the prior state. CancelIoEx aborts any in-flight OVERLAPPED op (connect /
  // accept / wait_disconnect); fire-and-forget ND2 Disconnect is issued only
  // from `connected`. Idempotent: second call is a no-op.
  void disconnect(implementation_type& impl, asio::error_code& ec) {
    if (!impl.connector_) {
      ec = rdma_errc::invalid_handle;
      ASIO_ERROR_LOCATION(ec);
      return;
    }

    auto old = impl.connect_state_.load(std::memory_order_acquire);
    while (old != connect_state::closed) {
      if (impl.connect_state_.compare_exchange_weak(old, connect_state::closed,
                                                    std::memory_order_acq_rel,
                                                    std::memory_order_acquire))
        break;
    }
    if (old == connect_state::closed) {
      ec.clear();
      return;
    }

    impl.peer_closed_.store(true, std::memory_order_release);

    switch (old) {
      case connect_state::idle:
        ec.clear();
        break;

      case connect_state::connecting:
        ::CancelIoEx(impl.connector_handle_.get(), NULL);
        ec.clear();
        break;

      case connect_state::connected: {
        ::CancelIoEx(impl.connector_handle_.get(), NULL);
        nd_disconnect_op::Handler h{};
        nd_disconnect_op::ptr p = {asio::detail::addressof(h),
                                   nd_disconnect_op::ptr::allocate(h), 0};
        p.p = new (p.v) nd_disconnect_op{impl.connector_.Get()};
        this->scheduler_.work_started();
        asio::error_code dec{};
        auto const hr = detail::disconnect(impl.connector_.Get(), p.p, dec);
        if (!dec && hr == ND_PENDING) {
          this->scheduler_.on_pending(p.p);
          ec.clear();
        }
        else {
          this->scheduler_.on_completion(p.p, dec);
          ec = dec;
        }
        p.v = p.p = 0;
        break;
      }

      default:
        ec.clear();
        break;
    }
  }

  // Disconnect NOTIFICATION (on_disconnect). One-shot; armed on demand via
  // IND2Connector::NotifyDisconnect. Level-triggered: completes immediately if
  // peer_closed_ OR connect_state_ == closed. Completion code:
  // rdma_errc::disconnected.
  template <typename Handler, typename IoExecutor>
  void async_wait_disconnect(implementation_type& impl, Handler& handler,
                             IoExecutor const& io_ex) {
    auto cancel_slot = asio::get_associated_cancellation_slot(handler);
    using op = nd_wait_disconnect_op<Handler, IoExecutor>;
    typename op::ptr p = {asio::detail::addressof(handler),
                          op::ptr::allocate(handler), 0};
    p.p =
        new (p.v) op{impl.connector_.Get(), &impl.peer_closed_, handler, io_ex};
    this->scheduler_.work_started();
    if (impl.peer_closed_.load(std::memory_order_acquire) ||
        impl.connect_state_.load(std::memory_order_acquire) ==
            connect_state::closed) {
      this->scheduler_.on_completion(p.p,
                                     make_error_code(rdma_errc::disconnected));
      p.v = p.p = 0;
      return;
    }
    if (!impl.connector_) {
      this->scheduler_.on_completion(
          p.p, make_error_code(rdma_errc::invalid_handle));
      p.v = p.p = 0;
      return;
    }
    asio::error_code ec{};
    auto const hr = detail::notify_disconnect(impl.connector_.Get(), p.p, ec);
    if (!ec && hr == ND_PENDING) {
      arm_nd_cancellation(cancel_slot, impl.connector_handle_.get(), p.p);
      this->scheduler_.on_pending(p.p);
    }
    else {
      this->scheduler_.on_completion(p.p, ec);
    }
    p.v = p.p = 0;
  }

 private:
  // Worker for both public open(ps, ...) and auto-open inside async_connect.
  void do_open(implementation_type& impl, asio::error_code& ec) {
    if (impl.connector_) {
      ec = asio::error::already_open;
      ASIO_ERROR_LOCATION(ec);
      return;
    }

    if (!device_svc_.is_registered()) {
      ec = rdma_errc::device_not_registered;
      ASIO_ERROR_LOCATION(ec);
      return;
    }

    auto adapter = device_svc_.get_device();
    impl.connector_handle_.reset(
        create_overlapped_file(adapter->adapter_.Get(), ec));
    if (ec) {
      ASIO_ERROR_LOCATION(ec);
      return;
    }

    impl.connector_.Attach(create_connector(adapter->adapter_.Get(),
                                            impl.connector_handle_.get(), ec));
    if (ec) {
      impl.connector_handle_.reset();
      ASIO_ERROR_LOCATION(ec);
      return;
    }

    this->scheduler_.register_handle(impl.connector_handle_.get(), ec);
    if (ec) {
      impl.connector_.Reset();
      impl.connector_handle_.reset();
      ASIO_ERROR_LOCATION(ec);
      return;
    }

    impl.adapter_ = adapter;
  }

  // device_service holds the device + effective config (installed by
  // use_device) and answers the registration guard. Cached as a ref in the
  // ctor.
  bool device_registered() { return device_svc_.is_registered(); }
  nd_config_t effective_config() {
    return device_svc_.is_registered() ? device_svc_.get_effective_config()
                                       : nd_config_t{};
  }

  void start_connect_op(implementation_type& impl, native_qp_t* qp,
                        endpoint_type const& endpoint,
                        asio::const_buffer private_data,
                        nd_connect_op_base* op) {
    this->scheduler_.work_started();
    // CAS idle -> connecting (if disconnect() raced to closed, abort).
    connect_state expected = connect_state::idle;
    if (!impl.connect_state_.compare_exchange_strong(
            expected, connect_state::connecting, std::memory_order_acq_rel,
            std::memory_order_acquire)) {
      this->scheduler_.on_completion(op, asio::error::operation_aborted);
      return;
    }

    asio::ip::address address;
    try {
      address = endpoint.address().is_v4() ? impl.adapter_->get_v4_address()
                                           : impl.adapter_->get_v6_address();
    } catch (std::system_error const& e) {
      this->scheduler_.on_completion(op, e.code());
      return;
    }
    asio::error_code ec{};
    endpoint_type local_endpoint{address, 0};
    bind_addr(impl.connector_.Get(), local_endpoint.data(),
              local_endpoint.size(), ec);
    if (ec) {
      this->scheduler_.on_completion(op, ec);
      return;
    }

    auto const eff = effective_config();
    auto const hr =
        connect(impl.connector_.Get(), qp, endpoint.data(), endpoint.size(),
                eff.inbound_read_limit_, eff.outbound_read_limit_,
                private_data.size() == 0 ? nullptr : private_data.data(),
                static_cast<ULONG>(private_data.size()), op, ec);
    if (ec) {
      this->scheduler_.on_completion(op, ec);
      return;
    }
    if (hr != ND_PENDING) {
      this->scheduler_.on_completion(op, ec);
      return;
    }
    this->scheduler_.on_pending(op);
  }

  void start_accept_op(implementation_type& impl, native_qp_t* qp,
                       asio::const_buffer private_data, nd_op_base* op) {
    this->scheduler_.work_started();
    // CAS idle -> connecting (if disconnect() raced to closed, abort).
    connect_state expected = connect_state::idle;
    if (!impl.connect_state_.compare_exchange_strong(
            expected, connect_state::connecting, std::memory_order_acq_rel,
            std::memory_order_acquire)) {
      this->scheduler_.on_completion(op, asio::error::operation_aborted);
      return;
    }
    asio::error_code ec{};
    auto const eff = effective_config();
    auto const hr =
        accept(impl.connector_.Get(), qp, eff.inbound_read_limit_,
               eff.outbound_read_limit_,
               private_data.size() == 0 ? nullptr : private_data.data(),
               static_cast<ULONG>(private_data.size()), op, ec);
    if (ec) {
      this->scheduler_.on_completion(op, ec);
      return;
    }
    if (hr != ND_PENDING) {
      this->scheduler_.on_completion(op, ec);
      return;
    }
    this->scheduler_.on_pending(op);
  }

  nd_device_service&
      device_svc_;  // cached (registration guard + device + conn params)
};

}  // namespace coro_io::detail
