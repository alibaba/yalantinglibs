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

#include "asio/ip/tcp.hpp"
#include "nd_connector.hpp"
#include "nd_listener.hpp"

namespace coro_io {

// Minimal RDMA port-space for the ND backend. Mirrors the original
// asio::rdma::tcp but ND-only (no backend selector, no ibv). Used to
// instantiate nd_connector<tcp> / nd_listener<tcp>; the services only need
// `endpoint` and `any_endpoint(port)` (plus v4()/v6() for open()).
class tcp {
 public:
  using endpoint = asio::ip::basic_endpoint<asio::ip::tcp>;
  using resolver = asio::ip::basic_resolver<asio::ip::tcp>;

  using connector = nd_connector<tcp>;
  using listener = nd_listener<tcp>;

 private:
  asio::ip::tcp impl_;
  explicit tcp(asio::ip::tcp impl) noexcept : impl_(impl) {}

 public:
  static tcp v4() noexcept { return tcp{asio::ip::tcp::v4()}; }
  static tcp v6() noexcept { return tcp{asio::ip::tcp::v6()}; }

  endpoint any_endpoint(asio::ip::port_type port) const noexcept {
    return endpoint{impl_, port};
  }
};

}  // namespace coro_io
