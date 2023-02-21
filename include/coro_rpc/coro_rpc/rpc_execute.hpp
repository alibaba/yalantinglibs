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
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

#include "connection.hpp"
#include "coro_connection.hpp"
#include "util/type_traits.h"

namespace coro_rpc::internal {
template <typename T, typename Conn>
concept has_get_reserve_size = requires(Conn &&conn) {
  T::get_reserve_size(conn);
};
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

template <typename rpc_protocol, typename serialize_proto,
          typename conn_return_type>
struct connection_responser {
  typename rpc_protocol::req_header req_head;
  void operator()(rpc_conn &&conn, conn_return_type &&ret) {
    if constexpr (has_get_reserve_size<rpc_protocol, rpc_conn>) {
      conn->response_msg<rpc_protocol>(
          serialize_proto::serialize(ret, rpc_protocol::get_reserve_size(conn)),
          req_head);
    }
    else {
      conn->response_msg<rpc_protocol>(serialize_proto::serialize(ret),
                                       req_head);
    }
  }
};

template <typename rpc_protocol, typename serialize_proto>
struct connection_responser<rpc_protocol, serialize_proto, void> {
  typename rpc_protocol::req_header req_head;
  void operator()(rpc_conn &&conn) {
    conn->response_msg<rpc_protocol>(serialize_proto::serialize(), req_head);
  }
};

using rpc_conn = std::shared_ptr<coro_connection>;
template <typename rpc_protocol, typename serialize_proto, auto func,
          typename Self = void>
inline std::optional<std::string> execute(
    std::string_view data, rpc_conn conn,
    typename rpc_protocol::req_header &req_head, Self *self = nullptr) {
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
        std::is_same_v<connection<conn_return_type>, First>;
    auto args = get_args<has_coro_conn_v, param_type>();

    bool is_ok = true;
    constexpr size_t size = std::tuple_size_v<decltype(args)>;
    if constexpr (size > 0) {
      is_ok = serialize_proto::deserialize_to(args, data);
    }

    if (!is_ok) [[unlikely]] {
      return std::nullopt;
    }

    if constexpr (std::is_void_v<return_type>) {
      if constexpr (std::is_void_v<Self>) {
        if constexpr (has_coro_conn_v) {
          // call void func(coro_conn, args...)
          std::apply(func,
                     std::tuple_cat(
                         std::forward_as_tuple(connection<conn_return_type>(
                             std::move(conn),
                             connection_responser<rpc_protocol, serialize_proto,
                                                  conn_return_type>{
                                 std::move(req_head)})),
                         std::move(args)));
        }
        else {
          // call void func(args...)
          std::apply(func, std::move(args));
        }
      }
      else {
        auto &o = *self;
        if constexpr (has_coro_conn_v) {
          // call void o.func(coro_conn, args...)
          std::apply(
              func,
              std::tuple_cat(
                  std::forward_as_tuple(
                      o, connection<conn_return_type>(
                             std::move(conn),
                             connection_responser<rpc_protocol, serialize_proto,
                                                  conn_return_type>{
                                 std::move(req_head)})),
                  std::move(args)));
        }
        else {
          // call void o.func(args...)
          std::apply(func,
                     std::tuple_cat(std::forward_as_tuple(o), std::move(args)));
        }
      }
    }
    else {
      if constexpr (std::is_void_v<Self>) {
        // call return_type func(args...)
        if constexpr (has_get_reserve_size<rpc_protocol, rpc_conn>) {
          return serialize_proto::serialize(
              std::apply(func, std::move(args)),
              rpc_protocol::get_reserve_size(conn));
        }
        else {
          return serialize_proto::serialize(std::apply(func, std::move(args)));
        }
      }
      else {
        auto &o = *self;
        // call return_type o.func(args...)
        if constexpr (has_get_reserve_size<rpc_protocol, rpc_conn>) {
          return serialize_proto::serialize(
              std::apply(func, std::tuple_cat(std::forward_as_tuple(o),
                                              std::move(args))),
              rpc_protocol::get_reserve_size(conn));
        }
        else {
          return serialize_proto::serialize(std::apply(
              func, std::tuple_cat(std::forward_as_tuple(o), std::move(args))));
        }
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
        if constexpr (has_get_reserve_size<rpc_protocol, rpc_conn>) {
          return serialize_proto::serialize(
              func(), rpc_protocol::get_reserve_size(conn));
        }
        else {
          return serialize_proto::serialize(func());
        }
      }
      else {
        if constexpr (has_get_reserve_size<rpc_protocol, rpc_conn>) {
          return serialize_proto::serialize(
              (self->*func)(), rpc_protocol::get_reserve_size(conn));
        }
        else {
          return serialize_proto::serialize((self->*func)());
        }
      }
    }
  }
  return serialize_proto::serialize();
}

template <typename rpc_protocol, typename serialize_proto, auto func,
          typename Self = void>
inline async_simple::coro::Lazy<std::optional<std::string>> execute_coro(
    std::string_view data, rpc_conn conn,
    typename rpc_protocol::req_header &req_head, Self *self = nullptr) {
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
        std::is_same_v<connection<conn_return_type>, First>;
    auto args = get_args<has_coro_conn_v, param_type>();

    bool is_ok = true;
    constexpr size_t size = std::tuple_size_v<decltype(args)>;
    if constexpr (size > 0) {
      is_ok = serialize_proto::deserialize_to(args, data);
    }

    if constexpr (std::is_void_v<return_type>) {
      if constexpr (std::is_void_v<Self>) {
        if constexpr (has_coro_conn_v) {
          // call void func(coro_conn, args...)
          co_await std::apply(
              func, std::tuple_cat(
                        std::forward_as_tuple(connection<conn_return_type>(
                            std::move(conn),
                            connection_responser<rpc_protocol, serialize_proto,
                                                 conn_return_type>{
                                std::move(req_head)})),
                        std::move(args)));
        }
        else {
          // call void func(args...)
          co_await std::apply(func, std::move(args));
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
                      o, connection<conn_return_type>(
                             std::move(conn),
                             connection_responser<rpc_protocol, serialize_proto,
                                                  conn_return_type>{
                                 std::move(req_head)})),
                  std::move(args)));
        }
        else {
          // call void o.func(args...)
          co_await std::apply(
              func, std::tuple_cat(std::forward_as_tuple(o), std::move(args)));
        }
      }
    }
    else {
      if constexpr (std::is_void_v<Self>) {
        // call return_type func(args...)
        if constexpr (has_get_reserve_size<rpc_protocol, rpc_conn>) {
          co_return serialize_proto::serialize(
              co_await std::apply(func, std::move(args)),
              rpc_protocol::get_reserve_size(conn));
        }
        else {
          co_return serialize_proto::serialize(
              co_await std::apply(func, std::move(args)));
        }
      }
      else {
        auto &o = *self;
        // call return_type o.func(args...)
        if constexpr (has_get_reserve_size<rpc_protocol, rpc_conn>) {
          co_return serialize_proto::serialize(
              co_await std::apply(func, std::tuple_cat(std::forward_as_tuple(o),
                                                       std::move(args))),
              rpc_protocol::get_reserve_size(conn));
        }
        else {
          co_return serialize_proto::serialize(co_await std::apply(
              func, std::tuple_cat(std::forward_as_tuple(o), std::move(args))));
        }
      }
    }
  }
  else {
    if constexpr (std::is_void_v<return_type>) {
      if constexpr (std::is_void_v<Self>) {
        co_await func();
      }
      else {
        // clang-format off
        co_await (self->*func)();
        // clang-format on
      }
    }
    else {
      if constexpr (std::is_void_v<Self>) {
        if constexpr (has_get_reserve_size<rpc_protocol, rpc_conn>) {
          co_return serialize_proto::serialize(
              co_await func(), rpc_protocol::get_reserve_size(conn));
        }
        else {
          co_return serialize_proto::serialize(co_await func());
        }
      }
      else {
        // clang-format off
        if constexpr (has_get_reserve_size<rpc_protocol, rpc_conn>) {
          co_return serialize_proto::serialize(co_await (self->*func)(), rpc_protocol::get_reserve_size(conn));
        }
        else {
          co_return serialize_proto::serialize(co_await (self->*func)());
        }
        // clang-format on
      }
    }
  }
  co_return serialize_proto::serialize();
}
}  // namespace coro_rpc::internal