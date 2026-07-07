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
#include <cassert>
#include <cstring>
#include <functional>
#include <iterator>
#include <optional>
#include <ranges>
#include <span>
#include <type_traits>
#include <utility>
#include <vector>

#include "asio/ip/tcp.hpp"
#include "ylt/coro_io/networkdirect/nd_types.hpp"
#include "ylt/coro_io/networkdirect/nd_error.hpp"
#include "nd_impl_types.hpp"

namespace coro_io::detail {

template <template <typename...> typename Container>
struct to_action {};

template <typename Fn>
struct filter_map_action {
  Fn fn;
};

template <typename KeyFn>
struct sort_by_action {
  KeyFn key_fn;
};

template <typename Pred>
struct chunk_by_action {
  Pred pred;
};

template <template <typename...> typename Container>
to_action<Container> to() {
  return {};
}

template <typename Fn>
filter_map_action<std::decay_t<Fn>> filter_map(Fn&& fn) {
  return filter_map_action<std::decay_t<Fn>>{std::forward<Fn>(fn)};
}

template <typename KeyFn>
sort_by_action<std::decay_t<KeyFn>> sort_by(KeyFn&& key_fn) {
  return sort_by_action<std::decay_t<KeyFn>>{std::forward<KeyFn>(key_fn)};
}

template <typename Pred>
chunk_by_action<std::decay_t<Pred>> chunk_by(Pred&& pred) {
  return chunk_by_action<std::decay_t<Pred>>{std::forward<Pred>(pred)};
}

template <std::ranges::input_range Range,
          template <typename...> typename Container>
auto operator|(Range&& range, to_action<Container>) {
  using value_type = std::ranges::range_value_t<Range>;

  Container<value_type> result;
  if constexpr (std::ranges::sized_range<Range>) {
    if constexpr (requires(Container<value_type>& c,
                           std::ranges::range_size_t<Range> n) {
                    c.reserve(n);
                  }) {
      result.reserve(std::ranges::size(range));
    }
  }

  std::ranges::copy(range, std::back_inserter(result));
  return result;
}

template <std::ranges::input_range Range, typename Fn>
auto operator|(Range&& range, filter_map_action<Fn> action) {
  using input_type = std::ranges::range_value_t<Range>;
  using optional_type = std::invoke_result_t<Fn&, input_type const&>;
  using output_type = std::decay_t<decltype(*std::declval<optional_type&>())>;

  std::vector<output_type> result;
  if constexpr (std::ranges::sized_range<Range>) {
    result.reserve(std::ranges::size(range));
  }

  for (input_type const& item : range) {
    auto value = std::invoke(action.fn, item);
    if (value) {
      result.push_back(std::move(*value));
    }
  }
  return result;
}

template <std::ranges::input_range Range, typename KeyFn>
auto operator|(Range&& range, sort_by_action<KeyFn> action) {
  auto result = std::forward<Range>(range) | to<std::vector>();
  std::ranges::sort(result, [&](auto const& a, auto const& b) {
    return std::invoke(action.key_fn, a) < std::invoke(action.key_fn, b);
  });
  return result;
}

template <std::ranges::input_range Range, typename Pred>
auto operator|(Range&& range, chunk_by_action<Pred> action) {
  using value_type = std::ranges::range_value_t<Range>;

  std::vector<std::vector<value_type>> chunks;
  if constexpr (std::ranges::sized_range<Range>) {
    chunks.reserve(std::ranges::size(range));
  }

  auto it = std::ranges::begin(range);
  auto const last = std::ranges::end(range);
  if (it == last) {
    return chunks;
  }

  std::vector<value_type> current;
  current.push_back(*it);
  auto previous = it++;
  for (; it != last; previous = it++) {
    if (std::invoke(action.pred, *previous, *it)) {
      current.push_back(*it);
    }
    else {
      chunks.push_back(std::move(current));
      current.clear();
      current.push_back(*it);
    }
  }
  chunks.push_back(std::move(current));
  return chunks;
}

struct resolved_nd_address {
  UINT64 adapter_id = 0;
  nd2_sockaddr_t address;
};

inline bool is_supported_addr_family(nd2_sockaddr_t const& addr) noexcept {
  auto const family = addr.src_addr_.sa_family;
  return family == AF_INET || family == AF_INET6;
}

inline std::optional<asio::ip::address> to_ip_address(
    nd2_sockaddr_t const& addr) {
  if (!is_supported_addr_family(addr)) {
    return std::nullopt;
  }
  asio::ip::tcp::endpoint endpoint;
  if (addr.address_size_ > endpoint.capacity()) {
    return std::nullopt;
  }
  std::memcpy(endpoint.data(), &addr.src_addr_, addr.address_size_);
  endpoint.resize(addr.address_size_);
  return endpoint.address();
}

inline std::vector<nd2_sockaddr_t> copy_socket_address_list(
    SOCKET_ADDRESS_LIST const& addr_list) {
  auto addr_range = std::ranges::subrange{
      addr_list.Address,
      addr_list.Address + addr_list.iAddressCount} |
      std::views::transform([](auto const& sock_addr) {
        nd2_sockaddr_t result{};
        if (static_cast<std::size_t>(sock_addr.iSockaddrLength) >
            sizeof(result.src_storage_)) {
          return result;
        }
        std::memcpy(&result.src_addr_, sock_addr.lpSockaddr,
                    sock_addr.iSockaddrLength);
        result.address_size_ = sock_addr.iSockaddrLength;
        return result;
      }) |
      std::views::filter([](auto const& addr) {
        return addr.address_size_ != 0;
      });

  return addr_range | to<std::vector>();
}

inline bool is_valid_proto(WSAPROTOCOL_INFOW const& proto) {
  constexpr auto fi_nd_proto_flag = XP1_GUARANTEED_DELIVERY |
                                    XP1_GUARANTEED_ORDER |
                                    XP1_MESSAGE_ORIENTED | XP1_CONNECT_DATA;
  if ((proto.dwServiceFlags1 & fi_nd_proto_flag) != fi_nd_proto_flag) {
    return false;
  }
  if (!(proto.iAddressFamily == AF_INET || proto.iAddressFamily == AF_INET6)) {
    return false;
  }
  if (proto.iSocketType != -1) {
    return false;
  }
  if (proto.iProtocol || proto.iProtocolMaxOffset) {
    return false;
  }
  return proto.iVersion == NDVER;
}

inline void enumerate_protos(std::vector<WSAPROTOCOL_INFOW>& out_protos,
                             asio::error_code& ec) {
  DWORD proto_len = 0;
  int err = 0;

  int number_info = ::WSCEnumProtocols(nullptr, nullptr, &proto_len, &err);
  if (number_info == SOCKET_ERROR && err != WSAENOBUFS) {
    ec = make_system_error_code(err);
    return;
  }

  std::size_t const array_size = proto_len / sizeof(WSAPROTOCOL_INFOW);
  std::vector<WSAPROTOCOL_INFOW> result{};
  result.resize(array_size);
  number_info = ::WSCEnumProtocols(nullptr, result.data(), &proto_len, &err);
  if (number_info == SOCKET_ERROR) {
    ec = make_system_error_code(err);
    return;
  }

  auto itr = std::remove_if(result.begin(), result.end(),
                            [](WSAPROTOCOL_INFOW const& elem) {
                              return !is_valid_proto(elem);
                            });
  result.erase(itr, result.end());

  out_protos = std::move(result);
  ec.clear();
}

inline std::vector<WSAPROTOCOL_INFOW> enumerate_protos() {
  std::vector<WSAPROTOCOL_INFOW> result{};
  std::error_code ec{};
  enumerate_protos(result, ec);
  throw_error(ec);
  return result;
}

inline std::wstring get_provider_path(WSAPROTOCOL_INFOW const& proto,
                                      asio::error_code& ec) {
  int len = 0, err = 0, res = 0;

  res = WSCGetProviderPath((GUID*)&proto.ProviderId, NULL, &len, &err);
  if (res != SOCKET_ERROR || err != WSAEFAULT) {
    ec = make_system_error_code(err);
    return {};
  }
  std::wstring temp{};
  temp.resize(len);
  res = WSCGetProviderPath((GUID*)&proto.ProviderId, temp.data(), &len, &err);
  if (res != 0) {
    if (err == WSAEINVAL) {
      ec = make_error_code(std::errc::invalid_argument);
    }
    else {
      ec = make_error_code(std::errc::not_enough_memory);
    }
    return {};
  }

  len = ExpandEnvironmentStringsW(temp.c_str(), NULL, 0);
  if (len == 0) {
    ec = make_system_error_code(::GetLastError());
    return {};
  }

  std::wstring result{};
  result.resize(len);
  len = ExpandEnvironmentStringsW(temp.c_str(), result.data(), len);
  if (len == 0) {
    ec = make_system_error_code(::GetLastError());
    return {};
  }

  return result;
}

inline auto create_provider_factory(std::wstring provider_path,
                                    WSAPROTOCOL_INFOW const& proto,
                                    asio::error_code& ec)
    -> nd_provider_factory_ptr {
  unique_module_t provier_module{LoadLibraryW(provider_path.c_str())};
  if (!provier_module) {
    ec = make_system_error_code(::GetLastError());
    return nullptr;
  }

  dll_can_unload_now unload = reinterpret_cast<dll_can_unload_now>(
      GetProcAddress(provier_module.get(), "DllCanUnloadNow"));
  if (!unload) {
    ec = make_system_error_code(::GetLastError());
    return nullptr;
  }

  dll_get_class_object getclassobj = reinterpret_cast<dll_get_class_object>(
      GetProcAddress(provier_module.get(), "DllGetClassObject"));
  if (!getclassobj) {
    ec = make_system_error_code(::GetLastError());
    return nullptr;
  }

  class_factory_ptr class_factory{};
  HRESULT hr =
      getclassobj(proto.ProviderId, IID_IClassFactory,
                  reinterpret_cast<LPVOID*>(class_factory.GetAddressOf()));
  if (hr != S_OK) {
    ec = make_system_error_code(hr);
    return nullptr;
  }

  auto provider_factory = std::make_shared<nd_provider_factory_t>();
  provider_factory->proto_ = proto;
  provider_factory->module_name_ = std::move(provider_path);
  provider_factory->module_ = std::move(provier_module);
  provider_factory->unload_ = unload;
  provider_factory->factory_ = std::move(class_factory);
  return provider_factory;
}

inline auto create_provider(nd_provider_factory_t const& factory,
                            asio::error_code& ec) -> nd2_provider_ptr {
  nd2_provider_ptr provdier{};
  HRESULT const hr = factory.factory_->CreateInstance(
      nullptr, IID_IND2Provider,
      reinterpret_cast<LPVOID*>(provdier.GetAddressOf()));
  if (hr != S_OK) {
    ec = make_system_error_code(hr);
    return nullptr;
  }
  return provdier;
}

inline void enumerate_addr_list(nd_provider_t const& provider,
                                std::vector<nd2_sockaddr_t>& addr_list,
                                asio::error_code& ec) {
  ULONG addr_list_buffer_size{0ul};
  provider.provider_->QueryAddressList(nullptr, &addr_list_buffer_size);
  if (addr_list_buffer_size == 0) {
    ec = make_error_code(std::errc::address_not_available);
    return;
  }

  scope_buffer buffer{std::malloc(addr_list_buffer_size)};
  if (!buffer.buffer) {
    ec = make_error_code(std::errc::not_enough_memory);
    return;
  }
  SOCKET_ADDRESS_LIST* temp_addr_list =
      static_cast<SOCKET_ADDRESS_LIST*>(buffer.buffer);
  HRESULT hr = provider.provider_->QueryAddressList(temp_addr_list,
                                                    &addr_list_buffer_size);
  if (hr != ND_SUCCESS) {
    ec = make_nd_error_code(hr);
    return;
  }
  if (temp_addr_list->iAddressCount <= 0) {
    ec = make_error_code(std::errc::address_not_available);
    return;
  }

  addr_list = copy_socket_address_list(*temp_addr_list);
  ec.clear();
}

inline auto enumerate_addr_list(nd_provider_t const& provider)
    -> std::vector<nd2_sockaddr_t> {
  std::vector<nd2_sockaddr_t> result{};
  std::error_code ec{};
  enumerate_addr_list(provider, result, ec);
  throw_error(ec);
  return result;
}

inline void query_adapter_addr_list(nd2_adapter_ptr const& adapter,
                                    std::vector<nd2_sockaddr_t>& addr_list,
                                    asio::error_code& ec) {
  assert(adapter);
  ULONG addr_list_buffer_size{0ul};
  adapter->QueryAddressList(nullptr, &addr_list_buffer_size);
  if (addr_list_buffer_size == 0) {
    ec = make_error_code(std::errc::address_not_available);
    return;
  }

  scope_buffer buffer{std::malloc(addr_list_buffer_size)};
  if (!buffer.buffer) {
    ec = make_error_code(std::errc::not_enough_memory);
    return;
  }
  SOCKET_ADDRESS_LIST* temp_addr_list =
      static_cast<SOCKET_ADDRESS_LIST*>(buffer.buffer);
  HRESULT hr =
      adapter->QueryAddressList(temp_addr_list, &addr_list_buffer_size);
  if (hr != ND_SUCCESS) {
    ec = make_nd_error_code(hr);
    return;
  }
  if (temp_addr_list->iAddressCount <= 0) {
    ec = make_error_code(std::errc::address_not_available);
    return;
  }

  addr_list = copy_socket_address_list(*temp_addr_list);
  ec.clear();
}

inline auto query_adapter_addr_list(nd2_adapter_ptr const& adapter)
    -> std::vector<nd2_sockaddr_t> {
  std::vector<nd2_sockaddr_t> result{};
  std::error_code ec{};
  query_adapter_addr_list(adapter, result, ec);
  throw_error(ec);
  return result;
}

inline UINT64 resolve_adapter_id(nd2_provider_ptr const& provider,
                                 sockaddr const* addrin,
                                 std::size_t addr_size,
                                 asio::error_code& ec) {
  UINT64 adapter_id = 0;
  HRESULT hr = provider->ResolveAddress(addrin, static_cast<ULONG>(addr_size),
                                        &adapter_id);
  if (hr != ND_SUCCESS) {
    ec = make_nd_error_code(hr);
    return 0;
  }
  return adapter_id;
}

inline ND2_ADAPTER_INFO query_adapter_info(nd2_adapter_ptr const& adaptor,
                                           asio::error_code& ec) {
  assert(adaptor);
  ND2_ADAPTER_INFO result = {0};
  result.InfoVersion = ND_VERSION_2;
  ULONG linfo = sizeof(result);
  HRESULT hr = adaptor->Query(&result, &linfo);
  if (hr != ND_SUCCESS) {
    ec = make_nd_error_code(hr);
  }
  return result;
}

inline std::string query_adapter_name(ND2_ADAPTER_INFO const& info,
                                      sockaddr const* addrin,
                                      std::size_t addr_size,
                                      asio::error_code& ec) {
  std::string result{};
  DWORD addrlen = 0;
#if defined(_MSC_VER) && (_MSC_VER >= 1800)
  int res =
      WSAAddressToStringW(const_cast<LPSOCKADDR>(addrin),
                          static_cast<DWORD>(addr_size), NULL, NULL, &addrlen);
  if (res != 0) {
    ec = make_system_error_code(::WSAGetLastError());
  }
  if (res == SOCKET_ERROR && ec.value() == WSAEFAULT && addrlen != 0) {
    LPWSTR string_buffer = (LPWSTR)_alloca(addrlen * sizeof(WCHAR));
    res = WSAAddressToStringW(const_cast<LPSOCKADDR>(addrin),
                              static_cast<DWORD>(addr_size), NULL,
                              string_buffer, &addrlen);
    if (res != 0) {
      ec = make_system_error_code(::WSAGetLastError());
    }
    else {
      ec.clear();
    }
    if (!ec) {
      res = ::WideCharToMultiByte(CP_ACP, 0, string_buffer, -1, NULL, 0, 0, 0);
      if (res == 0) {
        ec = make_system_error_code(GetLastError());
      }
      else {
        result.resize(res);
        res = ::WideCharToMultiByte(CP_ACP, 0, string_buffer, -1, result.data(),
                                    res, 0, 0);
        if (res == 0) {
          ec = make_system_error_code(::GetLastError());
        }
      }
    }
  }
#else
  int res =
      WSAAddressToStringA(const_cast<LPSOCKADDR>(addrin),
                          static_cast<DWORD>(addr_size), NULL, NULL, &addrlen);
  if (res != 0) {
    ec = make_system_error_code(::WSAGetLastError());
  }
  if (res == SOCKET_ERROR && ec.value() == WSAEFAULT && addrlen != 0) {
    result.resize(addrlen);
    res = WSAAddressToStringA(const_cast<LPSOCKADDR>(addrin),
                              static_cast<DWORD>(addr_size), NULL,
                              result.data(), &addrlen);
    if (res != 0) {
      ec = make_system_error_code(::WSAGetLastError());
    }
    else {
      ec.clear();
    }
  }
#endif
  return result;
}

inline HANDLE create_overlapped_file(native_context_t* context,
                                     asio::error_code& ec) {
  assert(context);
  HANDLE result{INVALID_HANDLE_VALUE};
  auto const hr = context->CreateOverlappedFile(&result);
  ec = static_cast<nd_errc>(hr);
  return result;
}

inline nd_adapter_ptr create_device(nd_provider_ptr const& provider,
                                    nd2_sockaddr_t const& addr,
                                    UINT64 adapter_id,
                                    asio::error_code& ec) {
  auto result = std::make_shared<nd_adapter_t>();
  nd2_adapter_ptr adapter_ptr{};
  HRESULT hr = provider->provider_->OpenAdapter(
      IID_IND2Adapter, adapter_id,
      reinterpret_cast<LPVOID*>(adapter_ptr.GetAddressOf()));
  if (hr != ND_SUCCESS) {
    ec = make_nd_error_code(hr);
    return nullptr;
  }
  auto const adapter_info = query_adapter_info(adapter_ptr, ec);
  if (ec) {
    return nullptr;
  }
  auto const adapter_name = query_adapter_name(
      adapter_info, &addr.src_addr_, addr.address_size_, ec);
  if (ec) {
    return nullptr;
  }
  auto pd = std::make_unique<native_pd_t>();
  pd->context_ = adapter_ptr.Get();
  pd->sync_handle_.reset(create_overlapped_file(adapter_ptr.Get(), ec));
  if (ec) {
    return nullptr;
  }
  result->adapter_ = adapter_ptr;
  result->pd_ = std::move(pd);
  result->adapter_id_ = adapter_id;
  result->name_ = adapter_name;
  result->info_ = adapter_info;
  return result;
}

inline void attach_device_addresses(
    nd_adapter_ptr const& device,
    std::span<nd2_sockaddr_t const> addresses) {
  assert(device);
  for (auto const& addr :
       addresses | std::views::filter(is_supported_addr_family)) {
    auto address = to_ip_address(addr);
    if (!address) {
      continue;
    }
    if (address->is_v4() && !device->v4_address_) {
      device->v4_address_ = *address;
    }
    else if (address->is_v6() && !device->v6_address_) {
      device->v6_address_ = *address;
    }
  }
}

inline nd_adapter_ptr open_device(
    nd_provider_ptr const& provider, UINT64 adapter_id,
    std::span<nd2_sockaddr_t const> provider_addresses,
    asio::error_code& ec) {
  if (provider_addresses.empty()) {
    ec = make_error_code(std::errc::invalid_argument);
    return nullptr;
  }

  auto device = create_device(provider, provider_addresses.front(),
                              adapter_id, ec);
  if (ec || !device) {
    return nullptr;
  }

  std::vector<nd2_sockaddr_t> adapter_addresses;
  asio::error_code query_ec;
  query_adapter_addr_list(device->adapter_, adapter_addresses, query_ec);
  if (!query_ec && !adapter_addresses.empty()) {
    attach_device_addresses(device, adapter_addresses);
  }
  else {
    attach_device_addresses(device, provider_addresses);
  }

  if (!device->v4_address_ && !device->v6_address_) {
    ec = make_error_code(std::errc::address_not_available);
    return nullptr;
  }

  ec.clear();
  return device;
}

inline std::vector<nd_adapter_ptr> discover_provider_devices(
    nd_provider_ptr const& provider) {
  std::vector<nd2_sockaddr_t> addr_list;
  asio::error_code enumerate_ec;
  enumerate_addr_list(*provider, addr_list, enumerate_ec);
  if (enumerate_ec) {
    return {};
  }

  return addr_list |
         std::views::filter(is_supported_addr_family) |
         filter_map([provider](nd2_sockaddr_t const& addr)
                        -> std::optional<resolved_nd_address> {
           asio::error_code resolve_ec;
           auto const id = resolve_adapter_id(
               provider->provider_, &addr.src_addr_, addr.address_size_,
               resolve_ec);
           if (resolve_ec) {
             return std::nullopt;
           }
           return resolved_nd_address{
               .adapter_id = id,
               .address = addr,
           };
         }) |
         sort_by([](resolved_nd_address const& item) {
           return item.adapter_id;
         }) |
         chunk_by([](resolved_nd_address const& a,
                     resolved_nd_address const& b) {
           return a.adapter_id == b.adapter_id;
         }) |
         std::views::transform([provider](auto const& chunk) {
           auto const adapter_id = chunk.front().adapter_id;
           auto provider_addresses =
               chunk |
               std::views::transform([](resolved_nd_address const& item) {
                 return item.address;
               }) |
               to<std::vector>();

           asio::error_code open_ec;
           return open_device(provider, adapter_id, provider_addresses,
                              open_ec);
         }) |
         std::views::filter([](nd_adapter_ptr const& device) {
           return device != nullptr;
         }) |
         to<std::vector>();
}

inline std::vector<nd_provider_ptr> get_providers(asio::error_code& ec) {
  std::vector<nd_provider_ptr> result{};
  std::vector<WSAPROTOCOL_INFOW> protos{};
  enumerate_protos(protos, ec);
  if (ec) {
    return result;
  }
  auto providers =
      protos |
      std::views::transform([](auto const& proto) {
        asio::error_code ec{};
        auto result = std::make_shared<nd_provider_t>();
        auto const provider_path = get_provider_path(proto, ec);
        if (ec) {
          return result;
        }
        auto provider_factory =
            create_provider_factory(provider_path, proto, ec);
        if (ec) {
          return result;
        }
        auto provider = create_provider(*provider_factory, ec);
        if (ec) {
          return result;
        }
        result->factory_ = provider_factory;
        result->provider_ = provider;
        return result;
      }) |
      std::views::filter([](auto const& provider) {
        return provider->provider_ != nullptr;
      });

  result = {std::ranges::begin(providers), std::ranges::end(providers)};
  if (result.empty()) {
    ec = rdma_errc::no_available_device;
  }
  return result;
}

inline std::vector<nd_provider_ptr> get_providers() {
  asio::error_code ec{};
  auto const result = get_providers(ec);
  throw_error(ec);
  return result;
}

inline void open_adapters(std::vector<nd_provider_ptr>& providers) {
  for (auto& provider : providers) {
    provider->devices_ = discover_provider_devices(provider);
  }
}

inline bool is_valid_adapter(nd_adapter_ptr const& adapter,
                             ND2_ADAPTER_INFO const& config) {
  if (!adapter) {
    return false;
  }
  auto const& capabilities = adapter->info_;
  if (config.MaxRegistrationSize != 0 &&
      config.MaxRegistrationSize > capabilities.MaxRegistrationSize) {
    return false;
  }
  if (config.MaxWindowSize != 0 &&
      config.MaxWindowSize > capabilities.MaxWindowSize) {
    return false;
  }
  if (config.MaxInitiatorSge != 0 &&
      config.MaxInitiatorSge > capabilities.MaxInitiatorSge) {
    return false;
  }
  if (config.MaxReceiveSge != 0 &&
      config.MaxReceiveSge > capabilities.MaxReceiveSge) {
    return false;
  }
  if (config.MaxReadSge != 0 &&
      config.MaxReadSge > capabilities.MaxReadSge) {
    return false;
  }
  if (config.MaxTransferLength != 0 &&
      config.MaxTransferLength > capabilities.MaxTransferLength) {
    return false;
  }
  if (config.MaxInlineDataSize != 0 &&
      config.MaxInlineDataSize > capabilities.MaxInlineDataSize) {
    return false;
  }
  if (config.MaxInboundReadLimit != 0 &&
      config.MaxInboundReadLimit > capabilities.MaxInboundReadLimit) {
    return false;
  }
  if (config.MaxOutboundReadLimit != 0 &&
      config.MaxOutboundReadLimit > capabilities.MaxOutboundReadLimit) {
    return false;
  }
  if (config.MaxReceiveQueueDepth != 0 &&
      config.MaxReceiveQueueDepth > capabilities.MaxReceiveQueueDepth) {
    return false;
  }
  if (config.MaxInitiatorQueueDepth != 0 &&
      config.MaxInitiatorQueueDepth > capabilities.MaxInitiatorQueueDepth) {
    return false;
  }
  if (config.MaxSharedReceiveQueueDepth != 0 &&
      config.MaxSharedReceiveQueueDepth >
          capabilities.MaxSharedReceiveQueueDepth) {
    return false;
  }
  if (config.MaxCompletionQueueDepth != 0 &&
      config.MaxCompletionQueueDepth > capabilities.MaxCompletionQueueDepth) {
    return false;
  }
  if (config.InlineRequestThreshold != 0 &&
      config.InlineRequestThreshold > capabilities.InlineRequestThreshold) {
    return false;
  }
  if (config.LargeRequestThreshold != 0 &&
      config.LargeRequestThreshold > capabilities.LargeRequestThreshold) {
    return false;
  }
  if (config.MaxCallerData != 0 &&
      config.MaxCallerData > capabilities.MaxCallerData) {
    return false;
  }
  if (config.MaxCalleeData != 0 &&
      config.MaxCalleeData > capabilities.MaxCalleeData) {
    return false;
  }
  if (config.AdapterFlags != 0) {
    if ((config.AdapterFlags & capabilities.AdapterFlags) !=
        config.AdapterFlags) {
      return false;
    }
  }
  return true;
}

inline bool is_valid_adapter(nd_adapter_ptr const& adapter,
                             nd_config_t const& config) {
  if (!adapter) {
    return false;
  }
  auto const& capabilities = adapter->info_;
  if (config.cqe_ > capabilities.MaxCompletionQueueDepth) {
    return false;
  }
  if (config.max_send_wr_ > capabilities.MaxInitiatorQueueDepth) {
    return false;
  }
  if (config.max_recv_wr_ > capabilities.MaxReceiveQueueDepth) {
    return false;
  }
  if (config.max_send_sge_ > capabilities.MaxInitiatorSge) {
    return false;
  }
  if (config.max_recv_sge_ > capabilities.MaxReceiveSge) {
    return false;
  }
  if (config.max_inline_data_ > capabilities.MaxInlineDataSize) {
    return false;
  }
  if (config.inbound_read_limit_ > capabilities.MaxInboundReadLimit) {
    return false;
  }
  if (config.outbound_read_limit_ > capabilities.MaxOutboundReadLimit) {
    return false;
  }
  return true;
}

inline bool is_valid_adapter(nd_adapter_ptr const& adapter) {
  return adapter != nullptr && adapter->adapter_ != nullptr;
}

}  // namespace coro_io::detail
