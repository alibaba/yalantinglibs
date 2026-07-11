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

#include <string>
#include <system_error>

#include "asio/error_code.hpp"
#include "networkdirect.hpp"

namespace coro_io {

enum class rdma_errc : int {
  no_available_device = 1,

  invalid_device,
  invalid_handle,

  already_registered,
  device_not_registered,

  disconnected,
  device_removed,
  connector_terminal,

  too_many_sge,
  private_data_too_large,

  address_family_not_supported,
};

class rdma_error_category : public std::error_category {
 public:
  inline const char* name() const noexcept override {
    return "rdma_error_code";
  }

  inline std::string message(int status) const override {
    switch (static_cast<rdma_errc>(status)) {
      case rdma_errc::no_available_device:
        return "RDMA no available device";
      case rdma_errc::invalid_device:
        return "RDMA invalid device";
      case rdma_errc::invalid_handle:
        return "RDMA invalid object handle";
      case rdma_errc::already_registered:
        return "RDMA device already registered on this execution context";
      case rdma_errc::device_not_registered:
        return "RDMA device not registered (call use_device first)";
      case rdma_errc::disconnected:
        return "RDMA connection disconnected";
      case rdma_errc::device_removed:
        return "RDMA local device removed";
      case rdma_errc::connector_terminal:
        return "RDMA connector is terminal; create a new connector";
      case rdma_errc::too_many_sge:
        return "RDMA scatter/gather list exceeds device max_sge";
      case rdma_errc::private_data_too_large:
        return "RDMA outgoing private_data exceeds the CM cap";
      case rdma_errc::address_family_not_supported:
        return "RDMA device has no local address of the requested family";
      default:
        return "UNKNOWN_RDMA_ERROR";
    }
  }
};

inline std::error_category const& get_rdma_error_category() {
  static rdma_error_category instance{};
  return instance;
}

inline std::error_code make_error_code(rdma_errc e) {
  return {static_cast<int>(e), get_rdma_error_category()};
}

enum class nd_errc : int {
  success = ND_SUCCESS,
  timeout = ND_TIMEOUT,
  buffer_overflow = ND_BUFFER_OVERFLOW,
  device_busy = ND_DEVICE_BUSY,
  no_more_entries = ND_NO_MORE_ENTRIES,
  unsuccessful = ND_UNSUCCESSFUL,
  access_violation = ND_ACCESS_VIOLATION,
  invalid_handle = ND_INVALID_HANDLE,
  invalid_device_request = ND_INVALID_DEVICE_REQUEST,
  invalid_parameter = ND_INVALID_PARAMETER,
  no_memory = ND_NO_MEMORY,
  invalid_parameter_mix = ND_INVALID_PARAMETER_MIX,
  data_overrun = ND_DATA_OVERRUN,
  sharing_violation = ND_SHARING_VIOLATION,
  insufficient_resources = ND_INSUFFICIENT_RESOURCES,
  device_not_ready = ND_DEVICE_NOT_READY,
  io_timeout = ND_IO_TIMEOUT,
  not_supported = ND_NOT_SUPPORTED,
  internal_error = ND_INTERNAL_ERROR,
  invalid_parameter_1 = ND_INVALID_PARAMETER_1,
  invalid_parameter_2 = ND_INVALID_PARAMETER_2,
  invalid_parameter_3 = ND_INVALID_PARAMETER_3,
  invalid_parameter_4 = ND_INVALID_PARAMETER_4,
  invalid_parameter_5 = ND_INVALID_PARAMETER_5,
  invalid_parameter_6 = ND_INVALID_PARAMETER_6,
  invalid_parameter_7 = ND_INVALID_PARAMETER_7,
  invalid_parameter_8 = ND_INVALID_PARAMETER_8,
  invalid_parameter_9 = ND_INVALID_PARAMETER_9,
  invalid_parameter_10 = ND_INVALID_PARAMETER_10,
  canceled = ND_CANCELED,
  remote_error = ND_REMOTE_ERROR,
  invalid_address = ND_INVALID_ADDRESS,
  invalid_device_state = ND_INVALID_DEVICE_STATE,
  invalid_buffer_size = ND_INVALID_BUFFER_SIZE,
  too_many_addresses = ND_TOO_MANY_ADDRESSES,
  address_already_exists = ND_ADDRESS_ALREADY_EXISTS,
  connection_refused = ND_CONNECTION_REFUSED,
  connection_invalid = ND_CONNECTION_INVALID,
  connection_active = ND_CONNECTION_ACTIVE,
  network_unreachable = ND_NETWORK_UNREACHABLE,
  host_unreachable = ND_HOST_UNREACHABLE,
  connection_aborted = ND_CONNECTION_ABORTED,
  device_removed = ND_DEVICE_REMOVED,
};

class nd_error_category : public std::error_category {
 public:
  inline const char* name() const noexcept override { return "nd_error_code"; }

  inline std::string message(int status) const override {
    switch (static_cast<HRESULT>(status)) {
      case ND_SUCCESS:
        return "ND_SUCCESS";
      case ND_TIMEOUT:
        return "ND_TIMEOUT";
      case ND_BUFFER_OVERFLOW:
        return "ND_BUFFER_OVERFLOW";
      case ND_DEVICE_BUSY:
        return "ND_DEVICE_BUSY";
      case ND_NO_MORE_ENTRIES:
        return "ND_NO_MORE_ENTRIES";
      case ND_UNSUCCESSFUL:
        return "ND_UNSUCCESSFUL";
      case ND_ACCESS_VIOLATION:
        return "ND_ACCESS_VIOLATION";
      case ND_INVALID_HANDLE:
        return "ND_INVALID_HANDLE";
      case ND_INVALID_DEVICE_REQUEST:
        return "ND_INVALID_DEVICE_REQUEST";
      case ND_INVALID_PARAMETER:
        return "ND_INVALID_PARAMETER";
      case ND_NO_MEMORY:
        return "ND_NO_MEMORY";
      case ND_INVALID_PARAMETER_MIX:
        return "ND_INVALID_PARAMETER_MIX";
      case ND_DATA_OVERRUN:
        return "ND_DATA_OVERRUN";
      case ND_SHARING_VIOLATION:
        return "ND_SHARING_VIOLATION";
      case ND_INSUFFICIENT_RESOURCES:
        return "ND_INSUFFICIENT_RESOURCES";
      case ND_DEVICE_NOT_READY:
        return "ND_DEVICE_NOT_READY";
      case ND_IO_TIMEOUT:
        return "ND_IO_TIMEOUT";
      case ND_NOT_SUPPORTED:
        return "ND_NOT_SUPPORTED";
      case ND_INTERNAL_ERROR:
        return "ND_INTERNAL_ERROR";
      case ND_INVALID_PARAMETER_1:
        return "ND_INVALID_PARAMETER_1";
      case ND_INVALID_PARAMETER_2:
        return "ND_INVALID_PARAMETER_2";
      case ND_INVALID_PARAMETER_3:
        return "ND_INVALID_PARAMETER_3";
      case ND_INVALID_PARAMETER_4:
        return "ND_INVALID_PARAMETER_4";
      case ND_INVALID_PARAMETER_5:
        return "ND_INVALID_PARAMETER_5";
      case ND_INVALID_PARAMETER_6:
        return "ND_INVALID_PARAMETER_6";
      case ND_INVALID_PARAMETER_7:
        return "ND_INVALID_PARAMETER_7";
      case ND_INVALID_PARAMETER_8:
        return "ND_INVALID_PARAMETER_8";
      case ND_INVALID_PARAMETER_9:
        return "ND_INVALID_PARAMETER_9";
      case ND_INVALID_PARAMETER_10:
        return "ND_INVALID_PARAMETER_10";
      case ND_CANCELED:
        return "ND_CANCELED";
      case ND_REMOTE_ERROR:
        return "ND_REMOTE_ERROR";
      case ND_INVALID_ADDRESS:
        return "ND_INVALID_ADDRESS";
      case ND_INVALID_DEVICE_STATE:
        return "ND_INVALID_DEVICE_STATE";
      case ND_INVALID_BUFFER_SIZE:
        return "ND_INVALID_BUFFER_SIZE";
      case ND_TOO_MANY_ADDRESSES:
        return "ND_TOO_MANY_ADDRESSES";
      case ND_ADDRESS_ALREADY_EXISTS:
        return "ND_ADDRESS_ALREADY_EXISTS";
      case ND_CONNECTION_REFUSED:
        return "ND_CONNECTION_REFUSED";
      case ND_CONNECTION_INVALID:
        return "ND_CONNECTION_INVALID";
      case ND_CONNECTION_ACTIVE:
        return "ND_CONNECTION_ACTIVE";
      case ND_NETWORK_UNREACHABLE:
        return "ND_NETWORK_UNREACHABLE";
      case ND_HOST_UNREACHABLE:
        return "ND_HOST_UNREACHABLE";
      case ND_CONNECTION_ABORTED:
        return "ND_CONNECTION_ABORTED";
      case ND_DEVICE_REMOVED:
        return "ND_DEVICE_REMOVED";
      default:
        return "UNKNOWN_ND_ERROR";
    }
  }
};

inline std::error_category const& get_nd_error_category() {
  static nd_error_category instance{};
  return instance;
}

inline std::error_code make_nd_error_code(int e) {
  return std::error_code{e, get_nd_error_category()};
}

inline std::error_code make_nd_error_code(HRESULT hr) {
  return std::error_code{static_cast<int>(hr), get_nd_error_category()};
}

inline std::error_code make_error_code(nd_errc e) {
  return make_nd_error_code(static_cast<int>(e));
}

inline std::error_code make_system_error_code(int e) {
  return std::error_code{e, std::system_category()};
}

inline void throw_error(std::error_code const& ec) {
  if (ec) {
    throw std::system_error(ec);
  }
}

}  // namespace coro_io

namespace std {
template <>
struct is_error_code_enum<coro_io::rdma_errc> : true_type {};
template <>
struct is_error_code_enum<coro_io::nd_errc> : true_type {};
}  // namespace std
