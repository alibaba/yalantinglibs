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
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <variant>

#include "connection.hpp"
#include "coro_connection.hpp"
#include "rpc_protocol.h"
#include "util/type_traits.h"

namespace coro_rpc::internal {

template <typename rpc_protocol>
inline auto pack_result(
    std::errc err, std::string_view err_msg,
    const typename rpc_protocol::supported_serialize_protocols
        &serialize_protocol) {
  return std::visit(
      [err,
       err_msg]<typename serialize_protocol_t>(const serialize_protocol_t &) {
        return std::make_pair(err, serialize_protocol_t::serialize(err_msg));
      },
      serialize_protocol);
}

template <typename serialize_proto>
inline auto pack_result(std::errc err, std::string_view err_msg) {
  return std::make_pair(err, serialize_proto::serialize(err_msg));
}

template <typename serialize_proto>
inline auto pack_result(const auto &ret) {
  return std::make_pair(std::errc{}, serialize_proto::serialize(ret));
}

template <typename serialize_proto>
inline auto pack_result(void) {
  return std::make_pair(std::errc{}, serialize_proto::serialize());
}

template <bool is_conn, typename First>
auto get_return_type() {
  if constexpr (is_conn) {
    using T = typename First::return_type;
    if constexpr (std::is_void_v<T>) {
      return;
    }
    else {
      return T{};
    }
  }
  else {
    return First{};
  }
}

template <typename serialize_proto, auto func, typename Self = void>
inline auto execute(std::string_view data, rpc_conn conn,
                    Self *self = nullptr) {
  using T = decltype(func);
  using param_type = function_parameters_t<T>;
  using return_type = function_return_type_t<T>;

  if constexpr (!std::is_void_v<param_type>) {
    using First = std::tuple_element_t<0, param_type>;
    constexpr bool is_conn = is_specialization<First, connection>::value;
    if constexpr (is_conn) {
      static_assert(std::is_void_v<return_type>,
                    "The return_type must be void");
    }

    using conn_return_type = decltype(get_return_type<is_conn, First>());
    constexpr bool has_coro_conn_v =
        std::is_same_v<connection<conn_return_type, coro_connection>, First>;
    auto args = get_args<has_coro_conn_v, param_type>();

    bool is_ok = true;
    constexpr size_t size = std::tuple_size_v<decltype(args)>;
    if constexpr (size > 0) {
      if constexpr (size == 1) {
        is_ok = serialize_proto::deserialize_to(std::get<0>(args), data);
      }
      else {
        is_ok = serialize_proto::deserialize_to(args, data);
      }
    }

    if (!is_ok) [[unlikely]] {
      return pack_result<serialize_proto>(std::errc::invalid_argument,
                                          "invalid arguments");
    }

    if constexpr (std::is_void_v<return_type>) {
      if constexpr (std::is_void_v<Self>) {
        if constexpr (has_coro_conn_v) {
          // call void func(coro_conn, args...)
          std::apply(
              func,
              std::tuple_cat(std::forward_as_tuple(connection<conn_return_type>(
                                 std::move(conn), serialize_proto::serialize)),
                             args));
        }
        else {
          // call void func(args...)
          std::apply(func, args);
        }
      }
      else {
        auto &o = *self;
        if constexpr (has_coro_conn_v) {
          // call void o.func(coro_conn, args...)
          std::apply(func,
                     std::tuple_cat(std::forward_as_tuple(
                                        o, connection<conn_return_type>(
                                               std::move(conn),
                                               serialize_proto::serialize)),
                                    args));
        }
        else {
          // call void o.func(args...)
          std::apply(func, std::tuple_cat(std::forward_as_tuple(o), args));
        }
      }
    }
    else {
      if constexpr (std::is_void_v<Self>) {
        // call return_type func(args...)
        return pack_result<serialize_proto>(std::apply(func, args));
      }
      else {
        auto &o = *self;
        // call return_type o.func(args...)
        return pack_result<serialize_proto>(
            std::apply(func, std::tuple_cat(std::forward_as_tuple(o), args)));
      }
    }
  }
  else {
    if constexpr (std::is_void_v<return_type>) {
      if constexpr (std::is_void_v<Self>) {
        func();
      }
      else {
        (self->*func)();
      }
    }
    else {
      if constexpr (std::is_void_v<Self>) {
        return pack_result<serialize_proto>(func());
      }
      else {
        return pack_result<serialize_proto>((self->*func)());
      }
    }
  }
  return pack_result<serialize_proto>();
}
// clang-format off
template <typename serialize_proto, auto func, typename Self = void>
inline async_simple::coro::Lazy<std::pair<std::errc, std::string>>
execute_coro(std::string_view data, rpc_conn conn, Self *self = nullptr) {
  using T = decltype(func);
  using param_type = function_parameters_t<T>;
  using return_type =
      typename get_type_t<typename function_return_type_t<T>::ValueType>::type;

  if constexpr (!std::is_void_v<param_type>) {
    using First = std::tuple_element_t<0, param_type>;
    constexpr bool is_conn = is_specialization<First, connection>::value;
    if constexpr (is_conn) {
      static_assert(std::is_void_v<return_type>,
                    "The return_type must be void");
    }

    using conn_return_type = decltype(get_return_type<is_conn, First>());
    constexpr bool has_coro_conn_v =
        std::is_same_v<connection<conn_return_type, coro_connection>, First>;
    auto args = get_args < has_coro_conn_v, param_type > ();

    bool is_ok = true;
    constexpr size_t size = std::tuple_size_v<decltype(args)>;
    if constexpr (size > 0) {
      if constexpr (size == 1) {
        is_ok = serialize_proto::deserialize_to(
            std::get<0>(args), data);
      }
      else {
        is_ok = serialize_proto::deserialize_to(args, data);
      }
    }

    if constexpr (std::is_void_v<return_type>) {
      if constexpr (std::is_void_v<Self>) {
        if constexpr (has_coro_conn_v) {
          // call void func(coro_conn, args...)
          co_await std::apply(
              func, std::tuple_cat(
                        std::forward_as_tuple(connection<conn_return_type>(std::move(conn),serialize_proto::serialize)),
                        args));
        }
        else {
          // call void func(args...)
          co_await std::apply(func, args);
        }
      }
      else {
        auto &o = *self;
        if constexpr (has_coro_conn_v) {
          // call void o.func(coro_conn, args...)
          co_await std::apply(
              func,
              std::tuple_cat(
                  std::forward_as_tuple(
                      o, connection<conn_return_type>(std::move(conn),serialize_proto::serialize_proto::serialize)),
                  args));
        }
        else {
          // call void o.func(args...)
          co_await std::apply(func,
                              std::tuple_cat(std::forward_as_tuple(o), args));
        }
      }
    }
    else {
      if constexpr (std::is_void_v<Self>) {
        // call return_type func(args...)
        co_return pack_result<serialize_proto>(co_await std::apply(func, args));
      }
      else {
        auto &o = *self;
        // call return_type o.func(args...)
        co_return pack_result<serialize_proto>(co_await std::apply(
            func, std::tuple_cat(std::forward_as_tuple(o), args)));
      }
    }
  }
  else {
    if constexpr (std::is_void_v<return_type>) {
      if constexpr (std::is_void_v<Self>) {
        co_await func();
      }
      else {
        co_await (self->*func)();
      }
    }
    else {
      if constexpr (std::is_void_v<Self>) {
        co_return pack_result<serialize_proto>(co_await func());
      }
      else {
        co_return pack_result<serialize_proto>(co_await (self->*func)());
      }
    }
  }
  co_return pack_result<serialize_proto>();
}
}  // namespace coro_rpc::internal