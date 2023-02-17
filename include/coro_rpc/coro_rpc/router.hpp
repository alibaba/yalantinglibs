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
#include <async_simple/coro/Lazy.h>
#include <util/type_traits.h>

#include <functional>
#include <memory>
#include <sstream>
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "easylog/easylog.h"
#include "rpc_execute.hpp"
#include "struct_pack/struct_pack/md5_constexpr.hpp"
#include "util/function_name.h"

namespace coro_rpc {
class coro_connection;
using rpc_conn = std::shared_ptr<coro_connection>;

namespace protocol {

template <typename rpc_protocol,
          template <typename...> typename map_t = std::unordered_map>

class router {
  using router_handler_t = std::function<std::optional<std::string>(
      std::string_view, rpc_conn, typename rpc_protocol::req_header &req_head,
      typename rpc_protocol::supported_serialize_protocols protocols)>;

  using coro_router_handler_t =
      std::function<async_simple::coro::Lazy<std::optional<std::string>>(
          std::string_view, rpc_conn,
          typename rpc_protocol::req_header &req_head,
          typename rpc_protocol::supported_serialize_protocols protocols)>;

  std::unordered_map<typename rpc_protocol::route_key_t, router_handler_t>
      handlers_;
  std::unordered_map<typename rpc_protocol::route_key_t, coro_router_handler_t>
      coro_handlers_;
  std::unordered_map<typename rpc_protocol::route_key_t, std::string> id2name_;

 private:
  const std::string &get_name(typename rpc_protocol::route_key_t key) {
    static std::string empty_string;
    if (auto it = id2name_.find(key); it != id2name_.end()) {
      return it->second;
    }
    else
      return empty_string;
  }

  // See https://gcc.gnu.org/bugzilla/show_bug.cgi?id=100611
  // We use this struct instead of lambda for workaround
  template <auto Func, typename Self>
  struct execute_visitor {
    std::string_view data;
    rpc_conn conn;
    typename rpc_protocol::req_header &req_head;
    Self *self;
    template <typename serialize_protocol>
    async_simple::coro::Lazy<std::optional<std::string>> operator()(
        const serialize_protocol &) {
      return internal::execute_coro<rpc_protocol, serialize_protocol, Func>(
          data, std::move(conn), req_head, self);
    }
  };

  template <auto Func>
  struct execute_visitor<Func, void> {
    std::string_view data;
    rpc_conn conn;
    typename rpc_protocol::req_header &req_head;
    template <typename serialize_protocol>
    async_simple::coro::Lazy<std::optional<std::string>> operator()(
        const serialize_protocol &) {
      return internal::execute_coro<rpc_protocol, serialize_protocol, Func>(
          data, std::move(conn), req_head);
    }
  };
  template <auto func, typename Self>
  void regist_one_handler(Self *self) {
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
          [self](
              std::string_view data, rpc_conn conn,
              typename rpc_protocol::req_header &req_head,
              typename rpc_protocol::supported_serialize_protocols protocols) {
            execute_visitor<func, Self> visitor{data, std::move(conn), req_head,
                                                self};
            return std::visit(visitor, protocols);
          });
      if (!it.second) {
        ELOGV(CRITICAL, "duplication function %s register!", name.data());
      }
    }
    else {
      auto it = handlers_.emplace(
          id,
          [self](
              std::string_view data, rpc_conn conn,
              typename rpc_protocol::req_header &req_head,
              typename rpc_protocol::supported_serialize_protocols protocols) {
            return std::visit(
                [data, conn = std::move(conn), self,
                 &req_head]<typename serialize_protocol>(
                    const serialize_protocol &obj) mutable {
                  return internal::execute<rpc_protocol, serialize_protocol,
                                           func>(data, std::move(conn),
                                                 req_head, self);
                },
                protocols);
          });
      if (!it.second) {
        ELOGV(CRITICAL, "duplication function %s register!", name.data());
      }
    }

    id2name_.emplace(id, name);
  }

  template <auto func>
  void regist_one_handler() {
    static_assert(!std::is_member_function_pointer_v<decltype(func)>,
                  "register member function but lack of the parent object");
    using return_type = function_return_type_t<decltype(func)>;

    constexpr auto name = get_func_name<func>();
    constexpr auto id =
        struct_pack::MD5::MD5Hash32Constexpr(name.data(), name.length());
    if constexpr (is_specialization_v<return_type, async_simple::coro::Lazy>) {
      auto it = coro_handlers_.emplace(id, [](std::string_view data,
                                              rpc_conn conn,
                                              typename rpc_protocol::req_header
                                                  &req_head,
                                              typename rpc_protocol::
                                                  supported_serialize_protocols
                                                      protocols) {
        execute_visitor<func, void> visitor{data, std::move(conn), req_head};
        return std::visit(visitor, protocols);
      });
      if (!it.second) {
        ELOGV(CRITICAL, "duplication function %s register!", name.data());
      }
    }
    else {
      auto it = handlers_.emplace(
          id,
          [](std::string_view data, rpc_conn conn,
             typename rpc_protocol::req_header &req_head,
             typename rpc_protocol::supported_serialize_protocols protocols) {
            return std::visit(
                [data, conn = std::move(conn),
                 &req_head]<typename serialize_protocol>(
                    const serialize_protocol &obj) {
                  return internal::execute<rpc_protocol, serialize_protocol,
                                           func>(data, std::move(conn),
                                                 req_head);
                },
                protocols);
          });
      if (!it.second) {
        ELOGV(CRITICAL, "duplication function %s register!", name.data());
      }
    }
    id2name_.emplace(id, name);
  }

 public:
  router_handler_t *get_handler(uint32_t id) {
    if (auto it = handlers_.find(id); it != handlers_.end()) {
      return &it->second;
    }
    return nullptr;
  }

  coro_router_handler_t *get_coro_handler(uint32_t id) {
    if (auto it = coro_handlers_.find(id); it != coro_handlers_.end()) {
      return &it->second;
    }
    return nullptr;
  }

  async_simple::coro::Lazy<std::pair<std::errc, std::string>> route_coro(
      auto handler, std::string_view data, rpc_conn conn,
      typename rpc_protocol::req_header &header,
      typename rpc_protocol::supported_serialize_protocols protocols,
      const typename rpc_protocol::route_key_t &route_key) {
    using namespace std::string_literals;
    if (handler) [[likely]] {
      try {
#ifndef NDEBUG
        ELOGV(INFO, "route function name: %s", get_name(route_key).data());

#endif
        // clang-format off
      auto res = co_await (*handler)(data, std::move(conn), header, protocols);
        // clang-format on
        if (res.has_value()) [[likely]] {
          co_return std::make_pair(std::errc{}, std::move(res.value()));
        }
        else {  // deserialize failed
          ELOGV(ERROR, "payload deserialize failed in rpc function: %s",
                get_name(route_key).data());
          co_return std::make_pair(std::errc::invalid_argument,
                                   "invalid rpc function arguments"s);
        }
      } catch (const std::exception &e) {
        ELOGV(ERROR, "exception: %s in rpc function: %s", e.what(),
              get_name(route_key).data());
        co_return std::make_pair(std::errc::interrupted, e.what());
      } catch (...) {
        ELOGV(ERROR, "unknown exception in rpc function: %s",
              get_name(route_key).data());
        co_return std::make_pair(std::errc::interrupted, "unknown exception"s);
      }
    }
    else {
      std::ostringstream ss;
      ss << route_key;
      ELOGV(ERROR, "the rpc function not registered, function ID: %s",
            ss.str().data());
      co_return std::make_pair(std::errc::function_not_supported,
                               "the rpc function not registered"s);
    }
  }

  std::pair<std::errc, std::string> route(
      auto handler, std::string_view data, rpc_conn conn,
      typename rpc_protocol::req_header &header,
      typename rpc_protocol::supported_serialize_protocols protocols,
      const typename rpc_protocol::route_key_t &route_key) {
    using namespace std::string_literals;
    if (handler) [[likely]] {
      try {
#ifndef NDEBUG
        ELOGV(INFO, "route function name: %s", get_name(route_key).data());
#endif
        auto res = (*handler)(data, std::move(conn), header, protocols);
        if (res.has_value()) [[likely]] {
          return std::make_pair(std::errc{}, std::move(res.value()));
        }
        else {  // deserialize failed
          ELOGV(ERROR, "payload deserialize failed in rpc function: %s",
                get_name(route_key).data());
          return std::make_pair(std::errc::invalid_argument,
                                "invalid rpc function arguments"s);
        }
      } catch (const std::exception &e) {
        ELOGV(ERROR, "exception: %s in rpc function: %s", e.what(),
              get_name(route_key).data());
        return std::make_pair(std::errc::interrupted, e.what());
      } catch (...) {
        ELOGV(ERROR, "unknown exception in rpc function: %s",
              get_name(route_key).data());
        return std::make_pair(std::errc::interrupted,
                              "unknown rpc function exception"s);
      }
    }
    else {
      std::ostringstream ss;
      ss << route_key;
      ELOGV(ERROR, "the rpc function not registered, function ID: %s",
            ss.str().data());
      return std::make_pair(std::errc::function_not_supported,
                            "the rpc function not registered"s);
    }
  }

  /*!
   * Register RPC service functions (member function)
   *
   * Before RPC server started, all RPC service functions must be registered.
   * All you need to do is fill in the template parameters with the address of
   * your own RPC functions. If RPC function is registered twice, the program
   * will be terminate with exit code `EXIT_FAILURE`.
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
  void regist_handler(class_type_t<decltype(first)> *self) {
    regist_one_handler<first>(self);
    (regist_one_handler<func>(self), ...);
  }

  /*!
   * Register RPC service functions (non-member function)
   *
   * Before RPC server started, all RPC service functions must be registered.
   * All you need to do is fill in the template parameters with the address of
   * your own RPC functions. If RPC function is registered twice, the program
   * will be terminate with exit code `EXIT_FAILURE`.
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
  void regist_handler() {
    regist_one_handler<first>();
    (regist_one_handler<func>(), ...);
  }
};

}  // namespace protocol
}  // namespace coro_rpc