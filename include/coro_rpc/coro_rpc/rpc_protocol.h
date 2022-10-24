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
#include <optional>
#include <string>
#include <system_error>
#if __has_include(<expected>) && __cplusplus > 202002L
#include <expected>
#if __cpp_lib_expected >= 202202L
#else
#include "util/expected.hpp"
#endif
#else
#include "util/expected.hpp"
#endif

namespace coro_rpc {
/*!
 * RPC header
 *
 * memory layout
 * ```
 * ┌────────┬───────────┬────────────────┬──────────┬─────────┬─────────┐
 * │  magic │  version  │ serialize_type │ msg_type │ seq_num │ length  │
 * ├────────┼───────────┼────────────────┼──────────┼─────────┼─────────┤
 * │   2    │     2     │       2        │    2     │    4    │    4    │
 * └────────┴───────────┴────────────────┴──────────┴─────────┴─────────┘
 * ```
 */
#pragma pack(1)
struct rpc_header {
  uint8_t magic;           //!< magic number
  uint8_t version;         //!< rpc protocol version
  uint8_t serialize_type;  //!< serialization type
  uint8_t msg_type;        //!< message type
  uint32_t seq_num;        //!< sequence number
  uint32_t length;         //!< length of RPC body
};
#pragma pack()

#if __cpp_lib_expected >= 202202L && __cplusplus > 202002L
template <class T, class E>
using expected = std::expected<T, E>;

template <class T>
using unexpected = std::unexpected<T>;

using unexpect_t = std::unexpect_t;

#else
template <class T, class E>
using expected = tl::expected<T, E>;

template <class T>
using unexpected = tl::unexpected<T>;

using unexpect_t = tl::unexpect_t;
#endif

constexpr int SIZE_OF_TYPES_CODE = 4;
constexpr int RPC_HEAD_LEN = sizeof(rpc_header) + SIZE_OF_TYPES_CODE;
constexpr int RESPONSE_HEADER_LEN = 4;
constexpr int FUNCTION_ID_LEN = 4;

constexpr int8_t magic_number = 21;

constexpr auto err_ok = std::errc{};
/*!
 * RPC error
 *
 * The `rpc_error` struct holds the error code `code` and error message `msg`.
 */
struct rpc_error {
  std::errc code;   //!< error code
  std::string msg;  //!< error message
};

template <typename T>
using rpc_result = expected<T, rpc_error>;

}  // namespace coro_rpc
