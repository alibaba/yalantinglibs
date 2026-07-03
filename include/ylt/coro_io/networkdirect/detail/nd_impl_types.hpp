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

#include <optional>

#include "asio/ip/address.hpp"
#include "nd_small_sglist.hpp"
#include "ylt/coro_io/networkdirect/nd_commons.hpp"

namespace coro_io::detail {

struct handle_deleter {
  void operator()(HANDLE handle) const {
    if (handle != INVALID_HANDLE_VALUE || handle != NULL) {
      ::CloseHandle(handle);
    }
  }
};
using unique_handle_t =
    std::unique_ptr<std::remove_pointer_t<HANDLE>, handle_deleter>;

struct module_deleter {
  void operator()(HMODULE module) const {
    if (module != NULL) {
      ::FreeLibrary(module);
    }
  }
};
using unique_module_t =
    std::unique_ptr<std::remove_pointer_t<HMODULE>, module_deleter>;

struct scope_buffer {
  void* buffer{nullptr};
  explicit scope_buffer(void* buffer_ptr) : buffer(buffer_ptr) {}
  ~scope_buffer() {
    if (buffer) {
      std::free(buffer);
    }
  }
};

using nd2_adapter_ptr = Microsoft::WRL::ComPtr<IND2Adapter>;
using nd2_provider_ptr = Microsoft::WRL::ComPtr<IND2Provider>;
using nd2_connector_ptr = Microsoft::WRL::ComPtr<IND2Connector>;
using nd2_listener_ptr = Microsoft::WRL::ComPtr<IND2Listener>;
using nd2_queue_pair_ptr = Microsoft::WRL::ComPtr<IND2QueuePair>;
using nd2_completion_queue_ptr = Microsoft::WRL::ComPtr<IND2CompletionQueue>;
using nd2_memory_region_ptr = Microsoft::WRL::ComPtr<IND2MemoryRegion>;
using class_factory_ptr = Microsoft::WRL::ComPtr<IClassFactory>;

using dll_can_unload_now = HRESULT (*)(void);
using dll_get_class_object = HRESULT (*)(REFCLSID rclsid, REFIID rrid,
                                         LPVOID* ppv);

using native_context_t = IND2Adapter;
using native_connector_t = IND2Connector;
using native_listener_t = IND2Listener;
using native_qp_t = IND2QueuePair;
using native_cq_t = IND2CompletionQueue;
using native_mr_t = IND2MemoryRegion;
using native_sge_t = ND2_SGE;
using native_wc_t = ND2_RESULT;
using native_context_config_t = ND2_ADAPTER_INFO;

struct native_pd_t {
  native_context_t* context_;
  unique_handle_t sync_handle_;
};

struct nd2_sockaddr_t {
  union {
    struct sockaddr src_addr_;
    struct sockaddr_in src_sin_;
    struct sockaddr_in6 src_sin6_;
    struct sockaddr_storage src_storage_;
  };
  size_t address_size_;
};

struct nd2_cq_init_attr {
  HANDLE overlapped_handle_;
  USHORT processor_group_;
  KAFFINITY processor_affinity_;
};

struct nd2_cq_notify_attr {
  ULONG type_;
  LPOVERLAPPED op_;
};

struct nd2_qp_init_attr {
  void* qp_context_;
  native_cq_t* rcq_;
  native_cq_t* icq_;
  ULONG max_send_wr_;
  ULONG max_recv_wr_;
  ULONG max_send_sge_;
  ULONG max_recv_sge_;
  ULONG max_inline_data_;
};

using native_qp_init_attr = nd2_qp_init_attr;
using native_cq_init_attr = nd2_cq_init_attr;
using native_cq_notify_attr = nd2_cq_notify_attr;

struct nd_provider_factory_t {
  WSAPROTOCOL_INFOW proto_;
  std::wstring module_name_;
  unique_module_t module_;
  dll_can_unload_now unload_;
  class_factory_ptr factory_;
};
using nd_provider_factory_ptr = std::shared_ptr<nd_provider_factory_t>;

struct nd_adapter_t {
  nd2_adapter_ptr adapter_;
  std::unique_ptr<native_pd_t> pd_;
  UINT64 adapter_id_ = 0;
  std::optional<asio::ip::address> v4_address_;
  std::optional<asio::ip::address> v6_address_;
  std::string name_;
  native_context_config_t info_;

  // Definitions live in nd_device.hpp (they need rdma_errc / error handling).
  asio::ip::address get_v4_address() const;
  asio::ip::address get_v6_address() const;
};
using nd_adapter_ptr = std::shared_ptr<nd_adapter_t>;

struct nd_provider_t {
  nd_provider_factory_ptr factory_;
  nd2_provider_ptr provider_;
  std::vector<nd_adapter_ptr> devices_;
};
using nd_provider_ptr = std::shared_ptr<nd_provider_t>;

struct nd_connector_state_t {
  unique_handle_t overlapped_handle_;
  nd2_connector_ptr connector_;
  nd2_completion_queue_ptr cq_;
  nd2_queue_pair_ptr qp_;
  nd_config_t config_;
  nd_adapter_ptr adapter_;
};
using nd_connector_state_ptr = std::shared_ptr<nd_connector_state_t>;

struct nd_connector_handle_t {
  nd2_connector_ptr connector_;
  unique_handle_t overlapped_handle_;
  nd_adapter_ptr adapter_;

  nd_connector_handle_t() = default;
  nd_connector_handle_t(nd_connector_handle_t&&) = default;
  nd_connector_handle_t& operator=(nd_connector_handle_t&&) = default;
  nd_connector_handle_t(nd_connector_handle_t const&) = delete;
  nd_connector_handle_t& operator=(nd_connector_handle_t const&) = delete;
};

inline constexpr std::size_t max_private_data_size = 256;
inline constexpr std::size_t max_outgoing_private_data = 255;

enum class connect_state : int {
  idle,
  connecting,
  connected,
  closed,
};

using nd_sglist_t = small_sglist<native_sge_t, 8>;

}  // namespace coro_io::detail
