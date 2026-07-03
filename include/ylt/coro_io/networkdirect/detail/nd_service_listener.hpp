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

#include <system_error>

#include "asio/associated_cancellation_slot.hpp"
#include "asio/detail/config.hpp"
#include "asio/detail/handler_alloc_helpers.hpp"
#include "asio/detail/memory.hpp"
#include "asio/ip/address.hpp"
#include "nd_service_base.hpp"
#include "nd_service_device.hpp"
#include "nd_ops_cm.hpp"
#include "nd_op_get_connection_request.hpp"
#include "nd_config_derive.hpp"

namespace coro_io::detail {

template <typename PortSpace>
class nd_listener_service
    : public asio::detail::execution_context_service_base<
          nd_listener_service<PortSpace>>
    , public nd_service_base {
public:
  using base_type = asio::detail::execution_context_service_base<
      nd_listener_service<PortSpace>>;
  using endpoint_type = typename PortSpace::endpoint;

  struct implementation_type : nd_service_base::base_implementation_type {
    nd2_listener_ptr listener_;
    unique_handle_t listener_handle_;
    nd_adapter_ptr adapter_;
    endpoint_type bind_endpoint_;
  };

  explicit nd_listener_service(asio::execution_context& ctx)
      : base_type(ctx)
      , nd_service_base(ctx)
      , device_svc_(asio::use_service<nd_device_service>(ctx)) {
  }

  ~nd_listener_service() = default;

  void shutdown() override {
    base_shutdown<implementation_type>([](implementation_type& impl) {
      impl.listener_.Reset();
      impl.listener_handle_.reset();
      impl.adapter_.reset();
    });
  }

  // lifecycle
  void construct(implementation_type& impl) {
    nd_service_base::base_construct(impl);
    impl.listener_.Reset();
    impl.listener_handle_.reset();
    impl.adapter_.reset();
    impl.bind_endpoint_ = endpoint_type{};
  }

  void destroy(implementation_type& impl) {
    impl.listener_.Reset();
    impl.listener_handle_.reset();
    impl.adapter_.reset();
    nd_service_base::base_destroy(impl);
  }

  void move_construct(implementation_type& impl,
                      implementation_type& other_impl) {
    nd_service_base::base_move_construct(impl, other_impl);
    impl.listener_ = std::move(other_impl.listener_);
    impl.listener_handle_ = std::move(other_impl.listener_handle_);
    impl.adapter_ = std::move(other_impl.adapter_);
    impl.bind_endpoint_ = other_impl.bind_endpoint_;
  }

  void move_assign(implementation_type& impl,
                   nd_listener_service& other_service,
                   implementation_type& other_impl) {
    impl.listener_.Reset();
    impl.listener_handle_.reset();
    nd_service_base::base_destroy(impl);
    nd_service_base::base_construct(impl);
    impl.listener_ = std::move(other_impl.listener_);
    impl.listener_handle_ = std::move(other_impl.listener_handle_);
    impl.adapter_ = std::move(other_impl.adapter_);
    impl.bind_endpoint_ = other_impl.bind_endpoint_;
  }

  // open: create listener + overlapped handle. The port-space value selects
  // which cached device address bind() will use.
  // Requires use_device() on this io_context.
  void open(implementation_type& impl, PortSpace const& ps,
            asio::error_code& ec) {
    if (impl.listener_) {
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
    impl.listener_handle_.reset(
        create_overlapped_file(adapter->adapter_.Get(), ec));
    if (ec) {
      ASIO_ERROR_LOCATION(ec);
      return;
    }

    impl.listener_.Attach(
        create_listener(adapter->adapter_.Get(),
                        impl.listener_handle_.get(), ec));
    if (ec) {
      impl.listener_handle_.reset();
      ASIO_ERROR_LOCATION(ec);
      return;
    }

    this->scheduler_.register_handle(impl.listener_handle_.get(), ec);
    if (ec) {
      impl.listener_.Reset();
      impl.listener_handle_.reset();
      ASIO_ERROR_LOCATION(ec);
      return;
    }

    impl.adapter_ = adapter;
    impl.bind_endpoint_ = ps.any_endpoint(0);
  }

  bool is_open(implementation_type const& impl) const noexcept {
    return impl.listener_ != nullptr;
  }

  void bind(implementation_type& impl, asio::ip::port_type port,
            asio::error_code& ec) {
    if (!is_open(impl)) {
      ec = rdma_errc::invalid_handle;
      ASIO_ERROR_LOCATION(ec);
      return;
    }
    asio::ip::address address;
    try {
      address = impl.bind_endpoint_.address().is_v4()
                    ? impl.adapter_->get_v4_address()
                    : impl.adapter_->get_v6_address();
    } catch (std::system_error const& e) {
      ec = e.code();
      ASIO_ERROR_LOCATION(ec);
      return;
    }

    endpoint_type endpoint{address, port};
    bind_addr(impl.listener_.Get(), endpoint.data(), endpoint.size(), ec);
    if (ec) {
      ASIO_ERROR_LOCATION(ec);
    }
  }

  void listen(implementation_type& impl, int backlog, asio::error_code& ec) {
    if (!is_open(impl)) {
      ec = rdma_errc::invalid_handle;
      ASIO_ERROR_LOCATION(ec);
      return;
    }
    detail::listen(impl.listener_.Get(), backlog, ec);
    if (ec) {
      ASIO_ERROR_LOCATION(ec);
    }
  }

  void cancel(implementation_type& impl) {
    if (impl.listener_handle_)
      ::CancelIoEx(impl.listener_handle_.get(), NULL);
  }

  // async get_connection_request
  template <typename Handler, typename IoExecutor>
  void async_get_connection_request(implementation_type& impl,
                                    Handler& handler, IoExecutor const& io_ex) {
    auto cancel_slot = asio::get_associated_cancellation_slot(handler);
    using op = nd_get_connection_request_op<Handler, IoExecutor>;

    asio::error_code ec;
    nd_connector_handle_t connector_handle;

    if (!is_open(impl) || !impl.adapter_) {
      typename op::ptr p = {asio::detail::addressof(handler),
                            op::ptr::allocate(handler), 0};
      p.p = new (p.v) op{impl.listener_.Get(),
                          std::move(connector_handle), handler, io_ex};
      this->scheduler_.work_started();
      this->scheduler_.on_completion(
          p.p, make_error_code(rdma_errc::invalid_handle));
      p.v = p.p = 0;
      return;
    }

    connector_handle.adapter_ = impl.adapter_;
    connector_handle.overlapped_handle_.reset(
        create_overlapped_file(impl.adapter_->adapter_.Get(), ec));
    if (ec) {
      typename op::ptr p = {asio::detail::addressof(handler),
                            op::ptr::allocate(handler), 0};
      p.p = new (p.v) op{impl.listener_.Get(),
                          std::move(connector_handle), handler, io_ex};
      this->scheduler_.work_started();
      this->scheduler_.on_completion(p.p, ec);
      p.v = p.p = 0;
      return;
    }

    connector_handle.connector_.Attach(
        create_connector(impl.adapter_->adapter_.Get(),
                         connector_handle.overlapped_handle_.get(), ec));
    if (ec) {
      typename op::ptr p = {asio::detail::addressof(handler),
                            op::ptr::allocate(handler), 0};
      p.p = new (p.v) op{impl.listener_.Get(),
                          std::move(connector_handle), handler, io_ex};
      this->scheduler_.work_started();
      this->scheduler_.on_completion(p.p, ec);
      p.v = p.p = 0;
      return;
    }

    auto* connector_ptr = connector_handle.connector_.Get();
    typename op::ptr p = {asio::detail::addressof(handler),
                          op::ptr::allocate(handler), 0};
    p.p = new (p.v) op{impl.listener_.Get(),
                        std::move(connector_handle), handler, io_ex};

    this->scheduler_.work_started();
    auto const hr =
        get_connection_request(impl.listener_.Get(), connector_ptr, p.p, ec);
    if (ec) {
      this->scheduler_.on_completion(p.p, ec);
    } else if (hr == ND_PENDING) {
      arm_nd_cancellation(cancel_slot, impl.listener_handle_.get(), p.p);
      this->scheduler_.on_pending(p.p);
    } else {
      this->scheduler_.on_completion(p.p, ec);
    }
    p.v = p.p = 0;
  }

  nd_device_service& device_svc_;  // cached (registration guard + device)
};

}  // namespace coro_io::detail
