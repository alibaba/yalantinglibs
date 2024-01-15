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

#include <cstdint>
#pragma once
namespace coro_rpc {
enum class errc : uint16_t {
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
  unknown_protocol_version,
  message_too_large,
  server_has_ran,
};
inline constexpr std::string_view make_error_message(errc ec) noexcept {
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
struct err_code {
 public:
  errc ec;
  constexpr err_code() noexcept : ec(errc::ok) {}
  explicit constexpr err_code(uint16_t ec) noexcept : ec{ec} {};
  constexpr err_code(errc ec) noexcept : ec(ec){};
  constexpr err_code& operator=(errc ec) noexcept {
    this->ec = ec;
    return *this;
  }
  constexpr err_code(const err_code& err_code) noexcept = default;
  constexpr err_code& operator=(const err_code& o) noexcept = default;
  constexpr operator errc() const noexcept { return ec; }
  constexpr operator bool() const noexcept { return static_cast<uint16_t>(ec); }
  constexpr explicit operator uint16_t() const noexcept {
    return static_cast<uint16_t>(ec);
  }
  constexpr uint16_t val() const noexcept { return static_cast<uint16_t>(ec); }
  constexpr std::string_view message() const noexcept {
    return make_error_message(ec);
  }
};

inline bool operator!(err_code ec) noexcept { return ec == errc::ok; }
inline bool operator!(errc ec) noexcept { return ec == errc::ok; }

};  // namespace coro_rpc