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

#include <ranges>
#include "asio/detail/throw_error.hpp"
#include "detail/nd_asio_manual_init.hpp"
#include "nd_types.hpp"
#include "nd_error.hpp"
#include "detail/nd_device_impl.hpp"

namespace coro_io {

using nd_device_t = detail::nd_adapter_t;
using nd_device_ptr = detail::nd_adapter_ptr;

class nd_device_manager_t {
 private:
  detail::nd_global_t global_;
  std::vector<detail::nd_provider_ptr> providers_;

  nd_device_manager_t()
    : global_()
    , providers_(detail::get_providers()) {
    detail::open_adapters(providers_);
  }

 public:
  static nd_device_manager_t const& instance() {
    static nd_device_manager_t instance{};
    return instance;
  }

  // Return the first device whose capabilities satisfy the (non-zero) config
  // constraints. A device may carry v4 and/or v6 local addresses; callers select
  // a concrete address with get_v4_address() / get_v6_address().
  nd_device_ptr get_first_available_device(nd_config_t const& config = {}) const {
    for (auto const& provider : providers_) {
      assert(provider);
      for (auto const& device : provider->devices_) {
        if (detail::is_valid_adapter(device, config)) {
          return device;
        }
      }
    }
    return nullptr;
  }

  template <typename Func>
  void for_each_device(Func&& func) const {
    for (auto const& provider : providers_) {
      assert(provider);
      for (auto const& device : provider->devices_) {
        if (!func(device)) return;
      }
    }
  }

 private:

};

namespace detail {

inline asio::ip::address nd_adapter_t::get_v4_address() const {
  if (!v4_address_) {
    asio::detail::throw_error(
        make_error_code(rdma_errc::address_family_not_supported));
  }
  return *v4_address_;
}

inline asio::ip::address nd_adapter_t::get_v6_address() const {
  if (!v6_address_) {
    asio::detail::throw_error(
        make_error_code(rdma_errc::address_family_not_supported));
  }
  return *v6_address_;
}

}  // namespace detail

}  // namespace coro_io
