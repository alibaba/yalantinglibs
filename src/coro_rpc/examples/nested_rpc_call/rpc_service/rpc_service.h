/*
 * Copyright (c) 2022, Alibaba Group Holding Limited;
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

#include <async_simple/coro/Lazy.h>

/*
 * Copyright (c) 2022, Alibaba Group Holding Limited;
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
#include <string_view>

#include "asio_util/asio_coro_util.hpp"
#include "asio_util/io_context_pool.hpp"
#include "coro_rpc/coro_rpc/coro_rpc_client.hpp"

inline std::string_view echo(std::string_view sv) { return sv; }

inline async_simple::coro::Lazy<std::string_view> nested_echo(
    std::string_view sv) {
  ELOGV(INFO, "start nested echo");
  coro_rpc::coro_rpc_client client(
      co_await asio_util::get_executor<asio::io_context>());
  [[maybe_unused]] auto ec = co_await client.connect("127.0.0.1", "8802");
  assert(ec == std::errc{});
  ELOGV(INFO, "connect another server");
  auto ret = co_await client.call<echo>(sv);
  assert(ret.value() == sv);
  ELOGV(INFO, "get echo result from another server");
  co_return sv;
}
