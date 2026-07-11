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

#include "asio/cancellation_type.hpp"
#include "ylt/coro_io/networkdirect/nd_error.hpp"
#include "ylt/coro_io/networkdirect/nd_types.hpp"

// interfaces simular with rdma-cm
namespace coro_io::detail {

// connector interfaces
inline IND2Connector* create_connector(IND2Adapter* adapter,
                                       HANDLE overlapped_handle,
                                       asio::error_code& ec) {
  assert(adapter);
  IND2Connector* result{nullptr};
  auto const hr = adapter->CreateConnector(IID_IND2Connector, overlapped_handle,
                                           reinterpret_cast<LPVOID*>(&result));
  ec = static_cast<nd_errc>(hr);
  return result;
}

inline result_type bind_addr(IND2Connector* connector, sockaddr const* addrin,
                             std::size_t addr_size, asio::error_code& ec) {
  assert(connector);
  auto const hr = connector->Bind(addrin, static_cast<size_type>(addr_size));
  if (hr != ND_SUCCESS) {
    // IND2Connector::Bind is specified as synchronous; keep this assert as a
    // defensive tripwire if a provider ever reports pending completion here.
    assert(hr != ND_PENDING);
  }
  ec = static_cast<nd_errc>(hr);
  return hr;
}

inline result_type accept(IND2Connector* connector, IND2QueuePair* qp,
                          ULONG inbound_read_limit, ULONG outbound_read_limit,
                          void const* private_data, ULONG private_data_size,
                          OVERLAPPED* overlapped, asio::error_code& ec) {
  assert(connector);
  assert(qp);
  auto const hr =
      connector->Accept(qp, inbound_read_limit, outbound_read_limit,
                        private_data, private_data_size, overlapped);

  if (hr != ND_SUCCESS && hr != ND_PENDING) {
    ec = static_cast<nd_errc>(hr);
  }
  else {
    ec.clear();
  }
  return hr;
}

inline result_type connect(IND2Connector* connector, IND2QueuePair* qp,
                           sockaddr const* addrin, size_t address_size,
                           ULONG inbound_read_limit, ULONG outbound_read_limit,
                           const void* private_data, ULONG data_size,
                           OVERLAPPED* overlapped, asio::error_code& ec) {
  assert(connector);
  assert(qp);
  auto const hr = connector->Connect(
      qp, addrin, static_cast<size_type>(address_size), inbound_read_limit,
      outbound_read_limit, private_data, data_size, overlapped);

  if (hr != ND_SUCCESS && hr != ND_PENDING) {
    ec = static_cast<nd_errc>(hr);
  }
  else {
    ec.clear();
  }
  return hr;
}

inline result_type disconnect(IND2Connector* connector, OVERLAPPED* overlapped,
                              asio::error_code& ec) {
  assert(connector);
  auto const hr = connector->Disconnect(overlapped);
  if (hr != ND_SUCCESS && hr != ND_PENDING) {
    ec = static_cast<nd_errc>(hr);
  }
  else {
    ec.clear();
  }
  return hr;
}

// Arm a disconnect NOTIFICATION: the overlapped completes when the connection
// is disconnected (peer or self). Backs async_wait_disconnect (on_disconnect).
inline result_type notify_disconnect(IND2Connector* connector,
                                     OVERLAPPED* overlapped,
                                     asio::error_code& ec) {
  assert(connector);
  auto const hr = connector->NotifyDisconnect(overlapped);
  if (hr != ND_SUCCESS && hr != ND_PENDING) {
    ec = static_cast<nd_errc>(hr);
  }
  else {
    ec.clear();
  }
  return hr;
}

// listener interfaces
inline IND2Listener* create_listener(IND2Adapter* adapter,
                                     HANDLE overlapped_handle,
                                     asio::error_code& ec) {
  assert(adapter);
  IND2Listener* result{nullptr};
  auto const hr = adapter->CreateListener(IID_IND2Listener, overlapped_handle,
                                          reinterpret_cast<LPVOID*>(&result));
  ec = static_cast<nd_errc>(hr);
  return result;
}

inline result_type listen(IND2Listener* listener, int backlog,
                          asio::error_code& ec) {
  assert(listener);
  auto const hr = listener->Listen(static_cast<ULONG>(backlog));
  ec = static_cast<nd_errc>(hr);
  return hr;
}

inline result_type get_connection_request(IND2Listener* listener,
                                          IND2Connector* connector,
                                          OVERLAPPED* overlapped,
                                          asio::error_code& ec) {
  assert(listener);
  assert(connector);
  auto const hr = listener->GetConnectionRequest(connector, overlapped);
  if (hr != ND_SUCCESS && hr != ND_PENDING) {
    ec = static_cast<nd_errc>(hr);
  }
  else {
    ec.clear();
  }
  return hr;
}

inline result_type bind_addr(IND2Listener* listener, sockaddr const* addrin,
                             std::size_t addr_size, asio::error_code& ec) {
  assert(listener);
  auto const hr = listener->Bind(addrin, static_cast<size_type>(addr_size));
  if (hr != ND_SUCCESS) {
    // IND2Listener::Bind is specified as synchronous; keep this assert as a
    // defensive tripwire if a provider ever reports pending completion here.
    assert(hr != ND_PENDING);
  }
  ec = static_cast<nd_errc>(hr);
  return hr;
}

// Per-op cancellation via CancelIoEx (mirrors ibv's cancel_ops_by_key pattern).
// Stored in the handler's cancellation_slot; when the slot fires (terminal /
// partial / total), CancelIoEx aborts the specific OVERLAPPED IO.
struct nd_cm_op_cancellation {
  HANDLE handle_;
  LPOVERLAPPED op_;

  void operator()(asio::cancellation_type_t type) {
    if (!!(type & (asio::cancellation_type::terminal |
                   asio::cancellation_type::partial |
                   asio::cancellation_type::total))) {
      ::CancelIoEx(handle_, op_);
    }
  }
};

template <typename CancellationSlot>
void arm_nd_cancellation(CancellationSlot slot, HANDLE handle,
                         LPOVERLAPPED op) {
  if (slot.is_connected()) {
    slot.template emplace<nd_cm_op_cancellation>(
        nd_cm_op_cancellation{handle, op});
  }
}

}  // namespace coro_io::detail
