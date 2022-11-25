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
#include <unordered_map>
#include <variant>

#include "async_connection.hpp"
#include "connection.hpp"
#include "coro_connection.hpp"
#include "logging/easylog.hpp"
#include "rpc_protocol.h"
#include "struct_pack/struct_pack.hpp"
#include "util/type_traits.h"

namespace coro_rpc {

namespace internal {

inline std::unordered_map<uint32_t,
                          std::function<std::pair<std::errc, std::vector<char>>(
                              std::string_view, rpc_conn &)>>
    g_handlers;
inline std::unordered_map<
    uint32_t, std::function<async_simple::coro::Lazy<std::pair<
                  std::errc, std::vector<char>>>(std::string_view, rpc_conn &)>>
    g_coro_handlers;
inline std::unordered_map<std::string, uint32_t> g_address2id;

inline std::unordered_map<uint32_t, std::string> g_id2name;

inline auto pack_result(std::errc err, std::string_view err_msg) {
  return std::make_pair(err, struct_pack::serialize_with_offset(
                                 /*offset = */ RESPONSE_HEADER_LEN,
                                 rpc_error{err, std::string{err_msg}}));
}

inline auto pack_result(const auto &ret) {
  return std::make_pair(std::errc{}, struct_pack::serialize_with_offset(
                                         RESPONSE_HEADER_LEN, ret));
}

inline auto pack_result(void) {
  return std::make_pair(std::errc{},
                        struct_pack::serialize_with_offset(RESPONSE_HEADER_LEN,
                                                           std::monostate{}));
}

inline std::function<std::pair<std::errc, std::vector<char>>(std::string_view,
                                                             rpc_conn &)>
    *get_handler(std::string_view data) {
  uint32_t id = *(uint32_t *)data.data();
  auto it = g_handlers.find(id);
  if (it != g_handlers.end()) {
    return &it->second;
  }
  return nullptr;
}

inline std::function<async_simple::coro::Lazy<
    std::pair<std::errc, std::vector<char>>>(std::string_view, rpc_conn &)>
    *get_coro_handler(std::string_view data) {
  uint32_t id = *(uint32_t *)data.data();
  auto it = g_coro_handlers.find(id);
  if (it != g_coro_handlers.end()) {
    return &it->second;
  }
  return nullptr;
}

inline async_simple::coro::Lazy<std::pair<std::errc, std::vector<char>>>
route_coro(auto handler, std::string_view data, rpc_conn conn) {
  uint32_t id = *(uint32_t *)data.data();
  if (handler) [[likely]] {
    try {
#ifndef NDEBUG
      if (auto it = internal::g_id2name.find(id);
          it != internal::g_id2name.end()) {
        easylog::info("route coro function name {}", it->second);
      }
#endif
      co_return co_await (*handler)(data, conn);
    } catch (const std::exception &e) {
      easylog::error("the rpc function has exception {}, function id {}",
                     e.what(), id);
      co_return pack_result(std::errc::interrupted, e.what());
    } catch (...) {
      easylog::error("the rpc function has unknown exception, function id {}",
                     id);
      co_return pack_result(std::errc::interrupted, "unknown exception");
    }
  }
  else [[unlikely]] {
    easylog::error("the rpc function not found, function id {}", id);
    co_return pack_result(std::errc::function_not_supported,
                          "the function not found");
  }
}

inline std::pair<std::errc, std::vector<char>> route(auto handler,
                                                     std::string_view data,
                                                     rpc_conn conn) {
  uint32_t id = *(uint32_t *)data.data();
  if (handler) [[likely]] {
    try {
#ifndef NDEBUG
      if (auto it = internal::g_id2name.find(id);
          it != internal::g_id2name.end()) {
        easylog::info("route function name {}", it->second);
      }
#endif
      return (*handler)(data, conn);
    } catch (const std::exception &e) {
      easylog::error("the rpc function has exception {}, function id {}",
                     e.what(), id);
      return pack_result(std::errc::interrupted, e.what());
    } catch (...) {
      easylog::error("the rpc function has unknown exception, function id {}",
                     id);
      return pack_result(std::errc::interrupted, "unknown exception");
    }
  }
  else [[unlikely]] {
    easylog::error("the rpc function not found, function id {}", id);
    return pack_result(std::errc::function_not_supported,
                       "the function not found");
  }
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

template <auto func, typename Self = void>
inline auto execute(std::string_view data, rpc_conn &conn,
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
    constexpr bool has_async_conn_v =
        std::is_same_v<connection<conn_return_type, async_connection>, First>;
    constexpr bool has_coro_conn_v =
        std::is_same_v<connection<conn_return_type, coro_connection>, First>;
    auto args = get_args < has_async_conn_v || has_coro_conn_v, param_type > ();

    struct_pack::errc err{};
    constexpr size_t size = std::tuple_size_v<decltype(args)>;
    if constexpr (size > 0) {
      data = data.substr(FUNCTION_ID_LEN);
      if constexpr (size == 1) {
        err = struct_pack::deserialize_to(std::get<0>(args), data.data(),
                                          data.size());
      }
      else {
        err = struct_pack::deserialize_to(args, data.data(), data.size());
      }
    }

    if (err != struct_pack::errc::ok) [[unlikely]] {
      return pack_result(std::errc::invalid_argument, "invalid arguments");
    }

    if constexpr (std::is_void_v<return_type>) {
      if constexpr (std::is_void_v<Self>) {
        if constexpr (has_async_conn_v) {
          // call void func(async_conn, args...)
          std::apply(
              func,
              std::tuple_cat(
                  std::forward_as_tuple(
                      connection<conn_return_type, async_connection>(
                          std::get<std::shared_ptr<async_connection>>(conn))),
                  args));
        }
        else if constexpr (has_coro_conn_v) {
          // call void func(coro_conn, args...)
          std::apply(func,
                     std::tuple_cat(
                         std::forward_as_tuple(connection<conn_return_type>(
                             std::get<std::shared_ptr<coro_connection>>(conn))),
                         args));
        }
        else {
          // call void func(args...)
          std::apply(func, args);
        }
      }
      else {
        auto &o = *self;
        if constexpr (has_async_conn_v) {
          // call void o.func(async_conn, args...)
          std::apply(
              func,
              std::tuple_cat(
                  std::forward_as_tuple(
                      o,
                      connection<conn_return_type, async_connection>(
                          std::get<std::shared_ptr<async_connection>>(conn))),
                  args));
        }
        else if constexpr (has_coro_conn_v) {
          // call void o.func(coro_conn, args...)
          std::apply(
              func,
              std::tuple_cat(
                  std::forward_as_tuple(
                      o, connnect<conn_return_type>(
                             std::get<std::shared_ptr<coro_connection>>(conn))),
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
        return pack_result(std::apply(func, args));
      }
      else {
        auto &o = *self;
        // call return_type o.func(args...)
        return pack_result(
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
        return pack_result(func());
      }
      else {
        return pack_result((self->*func)());
      }
    }
  }
  return pack_result();
}

template <auto func, typename Self = void>
inline async_simple::coro::Lazy<std::pair<std::errc, std::vector<char>>>
execute_coro(std::string_view data, rpc_conn &conn, Self *self = nullptr) {
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
    constexpr bool has_async_conn_v =
        std::is_same_v<connection<conn_return_type, async_connection>, First>;
    constexpr bool has_coro_conn_v =
        std::is_same_v<connection<conn_return_type, coro_connection>, First>;
    auto args = get_args < has_async_conn_v || has_coro_conn_v, param_type > ();

    struct_pack::errc err{};
    constexpr size_t size = std::tuple_size_v<decltype(args)>;
    if constexpr (size > 0) {
      data = data.substr(FUNCTION_ID_LEN);
      if constexpr (size == 1) {
        err = struct_pack::deserialize_to(std::get<0>(args), data.data(),
                                          data.size());
      }
      else {
        err = struct_pack::deserialize_to(args, data.data(), data.size());
      }
    }

    if (err != struct_pack::errc::ok) [[unlikely]] {
      co_return pack_result(std::errc::invalid_argument, "invalid arguments");
    }

    if constexpr (std::is_void_v<return_type>) {
      if constexpr (std::is_void_v<Self>) {
        if constexpr (has_async_conn_v) {
          // call void func(async_conn, args...)
          co_await std::apply(
              func,
              std::tuple_cat(
                  std::forward_as_tuple(
                      connection<conn_return_type, async_connection>(
                          std::get<std::shared_ptr<async_connection>>(conn))),
                  args));
        }
        else if constexpr (has_coro_conn_v) {
          // call void func(coro_conn, args...)
          co_await std::apply(
              func, std::tuple_cat(
                        std::forward_as_tuple(connection<conn_return_type>(
                            std::get<std::shared_ptr<coro_connection>>(conn))),
                        args));
        }
        else {
          // call void func(args...)
          co_await std::apply(func, args);
        }
      }
      else {
        auto &o = *self;
        if constexpr (has_async_conn_v) {
          // call void o.func(async_conn, args...)
          co_await std::apply(
              func,
              std::tuple_cat(
                  std::forward_as_tuple(
                      o,
                      connection<conn_return_type, async_connection>(
                          std::get<std::shared_ptr<async_connection>>(conn))),
                  args));
        }
        else if constexpr (has_coro_conn_v) {
          // call void o.func(coro_conn, args...)
          co_await std::apply(
              func,
              std::tuple_cat(
                  std::forward_as_tuple(
                      o, connnect<conn_return_type>(
                             std::get<std::shared_ptr<coro_connection>>(conn))),
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
        co_return pack_result(co_await std::apply(func, args));
      }
      else {
        auto &o = *self;
        // call return_type o.func(args...)
        co_return pack_result(co_await std::apply(
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
        co_await(self->*func)();
      }
    }
    else {
      if constexpr (std::is_void_v<Self>) {
        co_return pack_result(co_await func());
      }
      else {
        co_return pack_result(co_await(self->*func)());
      }
    }
  }
  co_return pack_result();
}

}  // namespace internal
}  // namespace coro_rpc
