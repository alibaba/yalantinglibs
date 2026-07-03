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

#include <cstddef>
#include <span>
#include <utility>

#include "asio/associator.hpp"
#include "asio/buffer.hpp"
#include "asio/detail/io_object_impl.hpp"
#include "asio/io_context.hpp"
#include "detail/nd_service_connector.hpp"
#include "nd_queue_pair.hpp"

namespace coro_io::detail {

// Adapts the full async_connect's void(ec, size_t reply_len) completion down to
// void(ec) for the no-reply convenience overload (drops reply_len). Associator-
// forwarding so the wrapped handler's cancellation_slot / allocator / executor
// still reach the underlying connect op. Mirrors ibv.
template <typename Handler>
struct nd_connect_drop_reply_adapter {
  Handler handler_;
  void operator()(asio::error_code ec, std::size_t /*reply_len*/) {
    std::move(handler_)(ec);
  }
};

}  // namespace coro_io::detail

namespace asio {

template <template <typename, typename> class Associator, typename Handler,
          typename Default>
struct associator<Associator,
                  coro_io::detail::nd_connect_drop_reply_adapter<Handler>,
                  Default> : Associator<Handler, Default> {
  static typename Associator<Handler, Default>::type get(
      coro_io::detail::nd_connect_drop_reply_adapter<Handler> const& a)
      noexcept {
    return Associator<Handler, Default>::get(a.handler_);
  }
  static auto get(
      coro_io::detail::nd_connect_drop_reply_adapter<Handler> const& a,
      Default const& d) noexcept
      -> decltype(Associator<Handler, Default>::get(a.handler_, d)) {
    return Associator<Handler, Default>::get(a.handler_, d);
  }
};

}  // namespace asio

namespace coro_io {

// Control-plane connector over NetworkDirect. Mirrors ibv_connector / asio's
// basic_socket:
//   - open(port_space)            create the IND2Connector (like socket.open(protocol))
//   - assign(native_handle)       adopt a connector handle from the listener
//   - async_connect(qp, ...)      Bind + Connect using qp.native_handle()
//   - async_accept(qp, ...)       Accept using qp.native_handle()
// The connector owns the IND2Connector; the queue_pair owns the QP. The QP is
// borrowed at connect/accept time (unlike ibv where the connector creates it).
template <typename PortSpace>
class nd_connector {
public:
  using service_type = detail::nd_connector_service<PortSpace>;
  using endpoint_type = typename PortSpace::endpoint;
  using native_connector_type = detail::nd_connector_handle_t;

  explicit nd_connector(asio::io_context& io_ctx)
      : impl_(0, 0, io_ctx) {
  }

  // opening constructor (mirrors basic_socket(io_context, protocol)). Requires
  // use_device() on this io_context (config is centralized there).
  nd_connector(asio::io_context& io_ctx, PortSpace const& ps)
      : impl_(0, 0, io_ctx) {
    asio::error_code ec;
    impl_.get_service().open(impl_.get_implementation(), ps, ec);
    asio::detail::throw_error(ec);
  }

  ~nd_connector() = default;
  nd_connector(nd_connector&&) = default;
  nd_connector& operator=(nd_connector&&) = default;
  nd_connector(nd_connector const&) = delete;
  nd_connector& operator=(nd_connector const&) = delete;

  // open: create the IND2Connector (client). The PortSpace is accepted for API
  // parity with ibv (and possible future v4/v6 selection); ND has no explicit
  // RDMA port space. Optional -- async_connect auto-opens if not already open.
  void open(PortSpace const& ps) {
    asio::error_code ec;
    open(ps, ec);
    asio::detail::throw_error(ec);
  }

  void open(PortSpace const& ps, asio::error_code& ec) {
    impl_.get_service().open(impl_.get_implementation(), ps, ec);
  }

  // assign: adopt a connector handle produced by the listener (server side).
  void assign(native_connector_type&& handle) {
    asio::error_code ec;
    assign(std::move(handle), ec);
    asio::detail::throw_error(ec);
  }

  void assign(native_connector_type&& handle, asio::error_code& ec) {
    impl_.get_service().assign(impl_.get_implementation(), std::move(handle),
                               ec);
  }

  bool is_open() const noexcept {
    return impl_.get_service().is_open(impl_.get_implementation());
  }

  // Private data is exchanged via the connect/accept/get_connection buffers
  // (request -> async_get_connection's buffer; reply -> async_connect's reply
  // buffer); there is no separate get_remote_data() accessor. Mirrors ibv.

  // async connect: Bind + Connect using qp.native_handle(). Sends
  // `outgoing_request` (copied; may be {}), receives the server's reply private
  // data into `incoming_reply` (may be {}). `outgoing_request` need not outlive
  // the call (copied into the op); `incoming_reply` must stay valid until
  // completion. handler(error_code, std::size_t reply_len).
  template <typename ConnectToken>
  auto async_connect(nd_queue_pair& qp, endpoint_type const& endpoint,
                     asio::const_buffer outgoing_request,
                     asio::mutable_buffer incoming_reply,
                     ConnectToken&& token) {
    return asio::async_initiate<ConnectToken,
                                void(asio::error_code, std::size_t)>(
        [this, &qp, &endpoint, outgoing_request, incoming_reply](auto handler) {
          auto io_ex = impl_.get_executor();
          impl_.get_service().async_connect(
              impl_.get_implementation(), qp.native_handle(), endpoint,
              outgoing_request, incoming_reply, handler, io_ex);
        },
        token);
  }

  // Convenience: connect without receiving the server's reply private data.
  // Completion is void(error_code) (no reply_len). Mirrors ibv.
  template <typename ConnectToken>
  auto async_connect(nd_queue_pair& qp, endpoint_type const& endpoint,
                     asio::const_buffer outgoing_request,
                     ConnectToken&& token) {
    return asio::async_initiate<ConnectToken, void(asio::error_code)>(
        [this, &qp, &endpoint, outgoing_request](auto handler) {
          auto io_ex = impl_.get_executor();
          detail::nd_connect_drop_reply_adapter<std::decay_t<decltype(handler)>>
              adapter{std::move(handler)};
          impl_.get_service().async_connect(
              impl_.get_implementation(), qp.native_handle(), endpoint,
              outgoing_request, asio::mutable_buffer{}, adapter, io_ex);
        },
        token);
  }

  // async accept: Accept using qp.native_handle(). handler(error_code)
  template <typename AcceptToken>
  auto async_accept(nd_queue_pair& qp,
                    asio::const_buffer outgoing_private_data,
                    AcceptToken&& token) {
    return asio::async_initiate<AcceptToken, void(asio::error_code)>(
        [this, &qp, outgoing_private_data](auto handler) {
          auto io_ex = impl_.get_executor();
          impl_.get_service().async_accept(impl_.get_implementation(),
                                           qp.native_handle(),
                                           outgoing_private_data, handler,
                                           io_ex);
        },
        token);
  }

  // Convenience: accept without sending reply private data. handler(error_code)
  template <typename AcceptToken>
  auto async_accept(nd_queue_pair& qp, AcceptToken&& token) {
    return async_accept(qp, asio::const_buffer{},
                        std::forward<AcceptToken>(token));
  }

  // disconnect: synchronous, non-blocking, abrupt teardown (mirrors socket
  // shutdown/close). ND2 Disconnect is overlapped, issued fire-and-forget; the
  // in-flight send/recv complete with operation_aborted ASYNCHRONOUSLY. To be
  // NOTIFIED of a (peer) disconnect, use async_wait_disconnect.
  void disconnect() {
    asio::error_code ec;
    disconnect(ec);
    asio::detail::throw_error(ec);
  }

  void disconnect(asio::error_code& ec) {
    impl_.get_service().disconnect(impl_.get_implementation(), ec);
  }

  // Disconnect NOTIFICATION (on_disconnect): one-shot, completes when the
  // connection is disconnected. handler(error_code) -- rdma_errc::disconnected. If
  // already disconnected, completes immediately (level-triggered). Mirrors ibv.
  template <typename WaitToken>
  auto async_wait_disconnect(WaitToken&& token) {
    return asio::async_initiate<WaitToken, void(asio::error_code)>(
        [this](auto handler) {
          auto io_ex = impl_.get_executor();
          impl_.get_service().async_wait_disconnect(impl_.get_implementation(),
                                                    handler, io_ex);
        },
        token);
  }

private:
  asio::detail::io_object_impl<service_type> impl_;
};

}  // namespace coro_io
