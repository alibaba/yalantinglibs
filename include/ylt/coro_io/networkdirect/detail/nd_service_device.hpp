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

#include "asio/execution_context.hpp"
#include "ylt/coro_io/networkdirect/nd_types.hpp"
#include "nd_impl_types.hpp"

namespace coro_io::detail {

// Per-io_context registration of the device (adapter) + effective config that
// use_device selected. Single responsibility: hold {device, effective_config}.
//
// Distinct from the process-wide nd_device_manager_t (device *discovery*) -- this
// is the per-io_context binding and the single source of truth for "use_device
// was called on this io_context" (is_registered()). The CQ/IOCP notify mechanism
// lives in the separate nd_io_completion_service.
class nd_device_service
    : public asio::detail::execution_context_service_base<nd_device_service> {
public:
  using base_type =
      asio::detail::execution_context_service_base<nd_device_service>;

  explicit nd_device_service(asio::execution_context& ctx) : base_type(ctx) {}

  void shutdown() override {
    device_.reset();
    registered_ = false;
  }

  void register_device(nd_adapter_ptr const& device,
                       nd_config_t const& effective) {
    device_ = device;
    effective_config_ = effective;
    registered_ = true;
  }

  bool is_registered() const noexcept { return registered_; }
  nd_adapter_ptr const& get_device() const noexcept { return device_; }
  nd_config_t const& get_effective_config() const noexcept {
    return effective_config_;
  }

private:
  nd_adapter_ptr device_;
  nd_config_t effective_config_{};
  bool registered_ = false;
};

}  // namespace coro_io::detail
