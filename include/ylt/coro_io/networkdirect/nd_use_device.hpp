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

#include "asio/io_context.hpp"
#include "nd_device.hpp"
#include "detail/nd_config_derive.hpp"
#include "detail/nd_service_device.hpp"
#include "detail/nd_service_io_completion.hpp"

namespace coro_io {

// Initialize the per-io_context shared-CQ service for an explicit device. The
// caller discovers the device beforehand via
// nd_device_manager_t::instance().get_first_available_device(config).
//
// Returns void: the caller already holds the device_ptr. The same device_ptr may
// be passed to use_device on multiple io_contexts. Mirrors ibv use_device.
inline void use_device(asio::io_context& io_ctx, nd_device_ptr const& device,
                       nd_config_t const& config, asio::error_code& ec) {
  auto& dev_svc = asio::use_service<detail::nd_device_service>(io_ctx);
  if (dev_svc.is_registered()) {
    ec = rdma_errc::already_registered;
    ASIO_ERROR_LOCATION(ec);
    return;
  }
  if (!device) {
    ec = rdma_errc::invalid_device;
    ASIO_ERROR_LOCATION(ec);
    return;
  }
  auto const effective = detail::derive_effective_config(config, device->info_);
  // Initialize the CQ/notify service first; register the device only on success.
  auto& io_svc = asio::use_service<detail::nd_io_completion_service>(io_ctx);
  io_svc.initialize(device, effective.cqe_, effective.cq_poll_batch_, ec);
  if (ec) {
    return;
  }
  dev_svc.register_device(device, effective);
}

inline void use_device(asio::io_context& io_ctx, nd_device_ptr const& device,
                       nd_config_t const& config = {}) {
  asio::error_code ec{};
  use_device(io_ctx, device, config, ec);
  asio::detail::throw_error(ec);
}

}  // namespace coro_io
