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
#include "easylog/easylog.h"
#include "rpc_execute.hpp"
#include "rpc_protocol.h"
#include "struct_pack/struct_pack.hpp"
#include "util/function_name.h"
#include "util/type_traits.h"
namespace coro_rpc::internal {

inline std::function<std::pair<std::errc, std::vector<char>>(std::string_view,
                                                             rpc_conn &)>
    *router::get_handler(std::string_view data) {
  uint32_t id = *(uint32_t *)data.data();
  auto it = handlers_.find(id);
  if (it != handlers_.end()) {
    return &it->second;
  }
  return nullptr;
}

inline std::function<async_simple::coro::Lazy<
    std::pair<std::errc, std::vector<char>>>(std::string_view, rpc_conn &)>
    *router::get_coro_handler(std::string_view data) {
  uint32_t id = *(uint32_t *)data.data();
  auto it = coro_handlers_.find(id);
  if (it != coro_handlers_.end()) {
    return &it->second;
  }
  return nullptr;
}

inline async_simple::coro::Lazy<std::pair<std::errc, std::vector<char>>>
router::route_coro(auto handler, std::string_view data, rpc_conn conn) {
  uint32_t id = *(uint32_t *)data.data();
  if (handler) [[likely]] {
    try {
#ifndef NDEBUG
      if (auto it = id2name_.find(id); it != id2name_.end()) {
        ELOGV(INFO, "route coro function name %d", it->second.data());
      }
#endif
      co_return co_await (*handler)(data, conn);
    } catch (const std::exception &e) {
      ELOGV(ERROR, "the rpc function has exception %s, function id %d",
            e.what(), id);
      co_return pack_result(std::errc::interrupted, e.what());
    } catch (...) {
      ELOGV(ERROR, "the rpc function has unknown exception, function id %d",
            id);
      co_return pack_result(std::errc::interrupted, "unknown exception");
    }
  }
  else [[unlikely]] {
    ELOGV(ERROR, "the rpc function not found, function id %d", id);
    co_return pack_result(std::errc::function_not_supported,
                          "the function not found");
  }
}

inline std::pair<std::errc, std::vector<char>> router::route(
    auto handler, std::string_view data, rpc_conn conn) {
  uint32_t id = *(uint32_t *)data.data();
  if (handler) [[likely]] {
    try {
#ifndef NDEBUG
      if (auto it = id2name_.find(id); it != id2name_.end()) {
        ELOGV(INFO, "route function name %s", it->second.data());
      }
#endif
      return (*handler)(data, conn);
    } catch (const std::exception &e) {
      ELOGV(ERROR, "the rpc function has exception %s, function id %d",
            e.what(), id);
      return pack_result(std::errc::interrupted, e.what());
    } catch (...) {
      ELOGV(ERROR, "the rpc function has unknown exception, function id %d",
            id);
      return pack_result(std::errc::interrupted, "unknown exception");
    }
  }
  else [[unlikely]] {
    ELOGV(ERROR, "the rpc function not found, function id %d", id);
    return pack_result(std::errc::function_not_supported,
                       "the function not found");
  }
}

template <auto func, typename Self>
inline void router::regist_one_handler(Self *self) {
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
        [self](std::string_view data, rpc_conn conn)
            -> async_simple::coro::Lazy<
                std::pair<std::errc, std::vector<char>>> {
          co_return co_await internal::execute_coro<func>(data, conn, self);
        });
    if (!it.second) {
      ELOGV(CRITICAL, "duplication function %s register!", name.data());
    }
  }
  else {
    auto it = handlers_.emplace(
        id, [self](std::string_view data, rpc_conn conn) mutable {
          return internal::execute<func>(data, conn, self);
        });
    if (!it.second) {
      ELOGV(CRITICAL, "duplication function %s register!", name.data());
    }
  }

  id2name_.emplace(id, name);
}

template <auto func>
inline void router::regist_one_handler() {
  static_assert(!std::is_member_function_pointer_v<decltype(func)>,
                "register member function but lack of the parent object");
  using return_type = function_return_type_t<decltype(func)>;

  constexpr auto name = get_func_name<func>();
  constexpr auto id =
      struct_pack::MD5::MD5Hash32Constexpr(name.data(), name.length());
  if constexpr (is_specialization_v<return_type, async_simple::coro::Lazy>) {
    auto it = coro_handlers_.emplace(
        id,
        [](std::string_view data, rpc_conn conn)
            -> async_simple::coro::Lazy<
                std::pair<std::errc, std::vector<char>>> {
          co_return co_await internal::execute_coro<func>(data, conn);
        });
    if (!it.second) {
      ELOGV(CRITICAL, "duplication function %s register!", name.data());
    }
  }
  else {
    auto it =
        handlers_.emplace(id, [](std::string_view data, rpc_conn conn) mutable {
          return internal::execute<func>(data, conn);
        });
    if (!it.second) {
      ELOGV(CRITICAL, "duplication function %s register!", name.data());
    }
  }
  id2name_.emplace(id, name);
}

template <auto func>
inline bool router::remove_handler() {
  constexpr auto name = get_func_name<func>();
  constexpr auto id =
      struct_pack::MD5::MD5Hash32Constexpr(name.data(), name.length());

  id2name_.erase(id);

  auto it = handlers_.find(id);
  if (it != handlers_.end()) {
    return handlers_.erase(id);
  }
  else {
    auto coro_it = coro_handlers_.find(id);
    if (coro_it != coro_handlers_.end()) {
      return coro_handlers_.erase(id);
    }

    return false;
  }
}

template <auto first, auto... func>
inline void router::regist_handler(class_type_t<decltype(first)> *self) {
  regist_one_handler<first>(self);
  (regist_one_handler<func>(self), ...);
}

template <auto first, auto... func>
inline void router::regist_handler() {
  regist_one_handler<first>();
  (regist_one_handler<func>(), ...);
}

inline void router::clear_handlers() {
  handlers_.clear();
  coro_handlers_.clear();
  id2name_.clear();
}

}  // namespace coro_rpc::internal
