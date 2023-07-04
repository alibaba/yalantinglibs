/*
 * Copyright (c) 2023, Alibaba Group Holding Limited;
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

#include <optional>
#include <system_error>
#include <variant>
#include <ylt/easylog.hpp>
#include <ylt/struct_pack.hpp>

#include "struct_pack_protocol.hpp"
#include "ylt/coro_io/coro_io.hpp"
#include "ylt/coro_rpc/impl/context.hpp"
#include "ylt/coro_rpc/impl/expected.hpp"
#include "ylt/coro_rpc/impl/router.hpp"

namespace coro_rpc {
namespace protocol {

struct coro_rpc_protocol {
 public:
  /*!
   * RPC header
   *
   * memory layout
   * ```
   * ┌────────┬───────────┬────────────────┬──────────┬─────────┬─────────┐─────────┐
   * │  magic │  version  │ serialize_type │ msg_type │ seq_num │ length
   * │reserved
   * ├────────┼───────────┼────────────────┼──────────┼─────────┼─────────┤─────────┤
   * │   1    │     1     │       1        │    1     │    4    │    4    │    4
   * │
   * └────────┴───────────┴────────────────┴──────────┴─────────┴─────────┘─────────┘
   * ```
   */
  struct req_header {
    uint8_t magic;           //!< magic number
    uint8_t version;         //!< rpc protocol version
    uint8_t serialize_type;  //!< serialization type
    uint8_t msg_type;        //!< message type
    uint32_t seq_num;        //!< sequence number
    uint32_t function_id;    //!< rpc function ID
    uint32_t length;         //!< length of RPC body
    uint32_t reserved;       //!< reserved field
  };

  struct resp_header {
    uint8_t magic;      //!< magic number
    uint8_t version;    //!< rpc protocol version
    uint8_t err_code;   //!< rpc error type
    uint8_t msg_type;   //!< message type
    uint32_t seq_num;   //!< sequence number
    uint32_t length;    //!< length of RPC body
    uint32_t reserved;  //!< reserved field
  };

  using buffer_type = std::vector<char>;

  using supported_serialize_protocols = std::variant<struct_pack_protocol>;
  using route_key_t = uint32_t;
  using router = coro_rpc::protocol::router<coro_rpc_protocol>;

  static std::optional<supported_serialize_protocols> get_serialize_protocol(
      req_header& req_header) {
    if (req_header.serialize_type == 0)
      AS_LIKELY { return struct_pack_protocol{}; }
    else {
      return std::nullopt;
    }
  };

  static route_key_t get_route_key(req_header& req_header) {
    return req_header.function_id;
  };

  template <typename Socket>
  static async_simple::coro::Lazy<std::error_code> read_head(
      Socket& socket, req_header& req_head) {
    // TODO: add a connection-level buffer in parameter to reuse memory
    auto [ec, _] = co_await coro_io::async_read(
        socket, asio::buffer((char*)&req_head, sizeof(req_header)));
    if (ec)
      AS_UNLIKELY { co_return std::move(ec); }
    else if (req_head.magic != magic_number)
      AS_UNLIKELY { co_return std::make_error_code(std::errc::protocol_error); }
    co_return std::error_code{};
  }

  template <typename Socket>
  static async_simple::coro::Lazy<std::error_code> read_payload(
      Socket& socket, req_header& req_head, buffer_type& buffer) {
    buffer.resize(req_head.length);
    auto [ec, _] = co_await coro_io::async_read(socket, asio::buffer(buffer));
    co_return ec;
  }

  static std::string prepare_response(std::string& rpc_result,
                                      const req_header& req_header,
                                      std::errc rpc_err_code = {},
                                      std::string_view err_msg = {}) {
    std::string header_buf;
    header_buf.resize(RESP_HEAD_LEN);
    auto& resp_head = *(resp_header*)header_buf.data();
    resp_head.magic = magic_number;
    resp_head.seq_num = req_header.seq_num;
    resp_head.err_code = static_cast<uint8_t>(rpc_err_code);
    if (rpc_err_code != std::errc{})
      AS_UNLIKELY {
        assert(rpc_result.empty());
        struct_pack::serialize_to(rpc_result, err_msg);
      }
    resp_head.length = rpc_result.size();
    return header_buf;
  }

  /*!
   * The RPC error for client
   *
   * The `rpc_error` struct holds the error code `code` and error message `msg`.
   */
  struct rpc_error {
    std::errc code;   //!< error code
    std::string msg;  //!< error message
  };

  // internal variable
  constexpr static inline int8_t magic_number = 21;

  static constexpr auto REQ_HEAD_LEN = sizeof(req_header{});
  static_assert(REQ_HEAD_LEN == 20);

  static constexpr auto RESP_HEAD_LEN = sizeof(resp_header{});
  static_assert(RESP_HEAD_LEN == 16);
};
}  // namespace protocol
template <typename return_msg_type>
using context = coro_rpc::context_base<return_msg_type,
                                       coro_rpc::protocol::coro_rpc_protocol>;
}  // namespace coro_rpc