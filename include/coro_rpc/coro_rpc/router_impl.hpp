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

#include "connection.hpp"
#include "coro_connection.hpp"
#include "coro_rpc/coro_rpc/router.hpp"
#include "easylog/easylog.h"
#include "rpc_execute.hpp"
#include "rpc_protocol.h"
#include "struct_pack/struct_pack/md5_constexpr.hpp"
#include "util/function_name.h"
#include "util/type_traits.h"
namespace coro_rpc::internal {

template <typename rpc_protocol>
inline std::function<std::pair<std::errc, std::string>(
    std::string_view, rpc_conn,
    typename rpc_protocol::supported_serialize_protocols protocols)>
    *router<rpc_protocol>::get_handler(uint32_t id) {
  if (auto it = handlers_.find(id); it != handlers_.end()) {
    return &it->second;
  }
  return nullptr;
}

template <typename rpc_protocol>
inline std::function<
    async_simple::coro::Lazy<std::pair<std::errc, std::string>>(
        std::string_view, rpc_conn,
        typename rpc_protocol::supported_serialize_protocols protocols)>
    *router<rpc_protocol>::get_coro_handler(uint32_t id) {
  if (auto it = coro_handlers_.find(id); it != coro_handlers_.end()) {
    return &it->second;
  }
  return nullptr;
}

template <typename rpc_protocol>
inline async_simple::coro::Lazy<std::pair<std::errc, std::string>>
router<rpc_protocol>::route_coro(
    uint32_t id, auto handler, std::string_view data, rpc_conn conn,
    typename rpc_protocol::supported_serialize_protocols protocols) {
  if (handler) [[likely]] {
    try {
#ifndef NDEBUG
      if (auto it = id2name_.find(id); it != id2name_.end()) {
        ELOGV(INFO, "route coro function name %s", it->second.data());
      }
#endif
      co_return co_await (*handler)(data, std::move(conn), protocols);
    } catch (const std::exception &e) {
      ELOGV(ERROR, "the rpc function has exception %s, function id %d",
            e.what(), id);
      co_return pack_result<rpc_protocol>(std::errc::interrupted, e.what(),
                                          protocols);
    } catch (...) {
      ELOGV(ERROR, "the rpc function has unknown exception, function id %d",
            id);
      co_return pack_result<rpc_protocol>(std::errc::interrupted,
                                          "unknown exception", protocols);
    }
  }
  else [[unlikely]] {
    ELOGV(ERROR, "the rpc function not found, function id %d", id);
    co_return pack_result<rpc_protocol>(std::errc::function_not_supported,
                                        "the function not found", protocols);
  }
}

template <typename rpc_protocol>
inline std::pair<std::errc, std::string> router<rpc_protocol>::route(
    uint32_t id, auto handler, std::string_view data, rpc_conn conn,
    typename rpc_protocol::supported_serialize_protocols protocols) {
  if (handler) [[likely]] {
    try {
#ifndef NDEBUG
      if (auto it = id2name_.find(id); it != id2name_.end()) {
        ELOGV(INFO, "route function name %s", it->second.data());
      }
#endif
      return (*handler)(data, std::move(conn), protocols);
    } catch (const std::exception &e) {
      ELOGV(ERROR, "the rpc function has exception %s, function id %d",
            e.what(), id);
      return pack_result<rpc_protocol>(std::errc::interrupted, e.what(),
                                       protocols);
    } catch (...) {
      ELOGV(ERROR, "the rpc function has unknown exception, function id %d",
            id);
      return pack_result<rpc_protocol>(std::errc::interrupted,
                                       "unknown exception", protocols);
    }
  }
  else [[unlikely]] {
    ELOGV(ERROR, "the rpc function not found, function id %d", id);
    return pack_result<rpc_protocol>(std::errc::function_not_supported,
                                     "the function not found", protocols);
  }
}

// See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=100611
// We use this struct instead of lambda for workaround
template <auto Func, typename Self>
struct execute_visitor {
  std::string_view data;
  rpc_conn conn;
  Self *self;
  template <typename serialize_protocol>
  async_simple::coro::Lazy<std::pair<std::errc, std::string>> operator()(
      const serialize_protocol &) {
    return internal::execute_coro<serialize_protocol, Func>(
        data, std::move(conn), self);
  }
};

template <auto Func>
struct execute_visitor<Func, void> {
  std::string_view data;
  rpc_conn conn;
  template <typename serialize_protocol>
  async_simple::coro::Lazy<std::pair<std::errc, std::string>> operator()(
      const serialize_protocol &) {
    return internal::execute_coro<serialize_protocol, Func>(data,
                                                            std::move(conn));
  }
};

template <typename rpc_protocol>
template <auto func, typename Self>
inline void router<rpc_protocol>::regist_one_handler(Self *self) {
  if (self == nullptr) [[unlikely]] {
    ELOGV(CRITICAL, "null connection!");
  }

  constexpr auto name = get_func_name<func>();
  constexpr auto id =
      struct_pack::MD5::MD5Hash32Constexpr(name.data(), name.length());
  using return_type = function_return_type_t<decltype(func)>;
  if constexpr (is_specialization_v<return_type, async_simple::coro::Lazy>) {
    auto it = coro_handlers_.emplace(
        id,
        [self](std::string_view data, rpc_conn conn,
               typename rpc_protocol::supported_serialize_protocols protocols) {
          execute_visitor<func, Self> visitor{data, std::move(conn), self};
          return std::visit(visitor, protocols);
        });
    if (!it.second) {
      ELOGV(CRITICAL, "duplication function %s register!", name.data());
    }
  }
  else {
    auto it = handlers_.emplace(
        id,
        [self](std::string_view data, rpc_conn conn,
               typename rpc_protocol::supported_serialize_protocols protocols) {
          return std::visit(
              [data, conn = std::move(conn), self]<typename serialize_protocol>(
                  const serialize_protocol &obj) mutable {
                return internal::execute<serialize_protocol, func>(
                    data, std::move(conn), self);
              },
              protocols);
        });
    if (!it.second) {
      ELOGV(CRITICAL, "duplication function %s register!", name.data());
    }
  }

  id2name_.emplace(id, name);
}

template <typename rpc_protocol>
template <auto func>
inline void router<rpc_protocol>::regist_one_handler() {
  static_assert(!std::is_member_function_pointer_v<decltype(func)>,
                "register member function but lack of the parent object");
  using return_type = function_return_type_t<decltype(func)>;

  constexpr auto name = get_func_name<func>();
  constexpr auto id =
      struct_pack::MD5::MD5Hash32Constexpr(name.data(), name.length());
  if constexpr (is_specialization_v<return_type, async_simple::coro::Lazy>) {
    auto it = coro_handlers_.emplace(
        id, [](std::string_view data, rpc_conn conn,
               typename rpc_protocol::supported_serialize_protocols protocols) {
          execute_visitor<func, void> visitor{data, std::move(conn)};
          return std::visit(visitor, protocols);
        });
    if (!it.second) {
      ELOGV(CRITICAL, "duplication function %s register!", name.data());
    }
  }
  else {
    auto it = handlers_.emplace(
        id, [](std::string_view data, rpc_conn conn,
               typename rpc_protocol::supported_serialize_protocols protocols) {
          return std::visit(
              [data, conn = std::move(conn)]<typename serialize_protocol>(
                  const serialize_protocol &obj) {
                return internal::execute<serialize_protocol, func>(
                    data, std::move(conn));
              },
              protocols);
        });
    if (!it.second) {
      ELOGV(CRITICAL, "duplication function %s register!", name.data());
    }
  }
  id2name_.emplace(id, name);
}

template <typename rpc_protocol>
template <auto first, auto... func>
inline void router<rpc_protocol>::regist_handler(
    class_type_t<decltype(first)> *self) {
  regist_one_handler<first>(self);
  (regist_one_handler<func>(self), ...);
}

template <typename rpc_protocol>
template <auto first, auto... func>
inline void router<rpc_protocol>::regist_handler() {
  regist_one_handler<first>();
  (regist_one_handler<func>(), ...);
}

}  // namespace coro_rpc::internal
