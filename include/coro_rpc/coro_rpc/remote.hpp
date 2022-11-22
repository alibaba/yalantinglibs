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
#include "router_impl.hpp"
#include "util/utils.hpp"
/*! \file remote.hpp
 *
 */
namespace coro_rpc {
namespace {
template <auto func, typename Self>
inline void register_one_handler(Self *self) {
  if (self == nullptr) [[unlikely]] {
    easylog::critical("null connection!");
  }

  constexpr auto name = get_func_name<func>();
  constexpr auto id =
      struct_pack::MD5::MD5Hash32Constexpr(name.data(), name.length());
  using return_type = function_return_type_t<decltype(func)>;
  if constexpr (is_specialization_v<return_type, async_simple::coro::Lazy>) {
    auto it = internal::g_coro_handlers.emplace(
        id,
        [self](std::string_view data, rpc_conn conn)
            -> async_simple::coro::Lazy<
                std::pair<std::errc, std::vector<char>>> {
          co_return co_await internal::execute_coro<func>(data, conn, self);
        });
    if (!it.second) {
      easylog::critical("duplication function {} register!", name);
    }
  }
  else {
    auto it = internal::g_handlers.emplace(
        id, [self](std::string_view data, rpc_conn conn) mutable {
          return internal::execute<func>(data, conn, self);
        });
    if (!it.second) {
      easylog::critical("duplication function {} register!", name);
    }
  }

  internal::g_id2name.emplace(id, name);
}

template <auto func>
inline void register_one_handler() {
  static_assert(!std::is_member_function_pointer_v<decltype(func)>,
                "register member function but lack of the parent object");
  using return_type = function_return_type_t<decltype(func)>;

  constexpr auto name = get_func_name<func>();
  constexpr auto id =
      struct_pack::MD5::MD5Hash32Constexpr(name.data(), name.length());
  if constexpr (is_specialization_v<return_type, async_simple::coro::Lazy>) {
    auto it = internal::g_coro_handlers.emplace(
        id,
        [](std::string_view data, rpc_conn conn)
            -> async_simple::coro::Lazy<
                std::pair<std::errc, std::vector<char>>> {
          co_return co_await internal::execute_coro<func>(data, conn);
        });
    if (!it.second) {
      easylog::critical("duplication function {} register!", name);
    }
  }
  else {
    auto it = internal::g_handlers.emplace(
        id, [](std::string_view data, rpc_conn conn) mutable {
          return internal::execute<func>(data, conn);
        });
    if (!it.second) {
      easylog::critical("duplication function {} register!", name);
    }
  }
  internal::g_id2name.emplace(id, name);
}
}  // namespace
/*!
 * Remove registered RPC function
 * @tparam func the address of RPC function. e.g. `&foo::bar`, `foobar`
 * @return true, if the function existed and removed success. otherwise, false.
 */
template <auto func>
inline bool remove_handler() {
  constexpr auto name = get_func_name<func>();
  constexpr auto id =
      struct_pack::MD5::MD5Hash32Constexpr(name.data(), name.length());

  internal::g_id2name.erase(id);

  auto it = internal::g_handlers.find(id);
  if (it != internal::g_handlers.end()) {
    return internal::g_handlers.erase(id);
  }
  else {
    auto coro_it = internal::g_coro_handlers.find(id);
    if (coro_it != internal::g_coro_handlers.end()) {
      return internal::g_coro_handlers.erase(id);
    }

    return false;
  }
}

/*!
 * Register RPC service functions (member function)
 *
 * Before RPC server started, all RPC service functions must be registered.
 * All you need to do is fill in the template parameters with the address of
 * your own RPC functions. If RPC function is registered twice, the program will
 * be terminate with exit code `EXIT_FAILURE`.
 *
 * Note: All functions must be member functions of the same class.
 *
 * ```cpp
 * class test_class {
 *  public:
 *  void plus_one(int val) {}
 *  std::string get_str(std::string str) { return str; }
 * };
 * int main() {
 *   test_class obj{};
 *   // register member functions
 *   register_handler<&test_class::plus_one, &test_class::get_str>(&obj);
 *   return 0;
 * }
 * ```
 *
 * @tparam first the address of RPC function. e.g. `&foo::bar`
 * @tparam func the address of RPC function. e.g. `&foo::bar`
 * @param self the object pointer corresponding to these member functions
 */
template <auto first, auto... func>
inline void register_handler(class_type_t<decltype(first)> *self) {
  register_one_handler<first>(self);
  (register_one_handler<func>(self), ...);
}

/*!
 * Register RPC service functions (non-member function)
 *
 * Before RPC server started, all RPC service functions must be registered.
 * All you need to do is fill in the template parameters with the address of
 * your own RPC functions. If RPC function is registered twice, the program will
 * be terminate with exit code `EXIT_FAILURE`.
 *
 * ```cpp
 * // RPC functions (non-member function)
 * void hello() {}
 * std::string get_str() { return ""; }
 * int add(int a, int b) {return a + b; }
 * int main() {
 *   register_handler<hello>();         // register one RPC function at once
 *   register_handler<get_str, add>();  // register multiple RPC functions at
 * once return 0;
 * }
 * ```
 *
 * @tparam first the address of RPC function. e.g. `foo`, `bar`
 * @tparam func the address of RPC function. e.g. `foo`, `bar`
 */
template <auto first, auto... func>
inline void register_handler() {
  register_one_handler<first>();
  (register_one_handler<func>(), ...);
}

inline void clear_handlers() {
  internal::g_handlers.clear();
  internal::g_coro_handlers.clear();
  internal::g_id2name.clear();
}
}  // namespace coro_rpc
