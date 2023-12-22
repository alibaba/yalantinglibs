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
#include <ylt/struct_pack/util.h>
#pragma once
namespace coro_rpc {
enum class errc : uint8_t {
  ok,
  io_error,
  not_connected,
  timed_out,
  invalid_argument,
  address_in_use,
  operation_canceled,
  interrupted,
  function_not_registered,
  protocol_error,
  message_too_large,
  server_has_ran,
  user_defined_err_min = 100,
  user_defined_err_max = 255
};
inline bool operator!(errc ec) { return ec == errc::ok; }
inline std::string_view make_error_message(errc ec) {
  switch (ec) {
    case errc::ok:
      return "ok";
    case errc::io_error:
      return "io_error";
    case errc::not_connected:
      return "not_connected";
    case errc::timed_out:
      return "timed_out";
    case errc::invalid_argument:
      return "invalid_argument";
    case errc::address_in_use:
      return "address_in_use";
    case errc::operation_canceled:
      return "operation_canceled";
    case errc::interrupted:
      return "interrupted";
    case errc::function_not_registered:
      return "function_not_registered";
    case errc::protocol_error:
      return "protocol_error";
    case errc::message_too_large:
      return "message_too_large";
    case errc::server_has_ran:
      return "server_has_ran";
    default:
      return "unknown_user-defined_error";
  }
}
};  // namespace coro_rpc