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
#include <string_view>
#include <unordered_map>
#include <variant>
#include <vector>

#include "protocol.hpp"

namespace coro_rpc {
class coro_connection;
using rpc_conn = std::shared_ptr<coro_connection>;

namespace protocol {

class router {
  using rpc_protocol = coro_rpc::protocol::coro_rpc_protocol;
  std::unordered_map<
      uint32_t,
      std::function<std::optional<std::string>(
          std::string_view, rpc_conn, rpc_protocol::req_header &req_head,
          rpc_protocol::supported_serialize_protocols protocols)>>
      handlers_;
  std::unordered_map<
      uint32_t,
      std::function<async_simple::coro::Lazy<std::optional<std::string>>(
          std::string_view, rpc_conn, rpc_protocol::req_header &req_head,
          rpc_protocol::supported_serialize_protocols protocols)>>
      coro_handlers_;
  std::unordered_map<uint32_t, std::string> id2name_;

  template <auto func, typename Self>
  void regist_one_handler(Self *self);

  template <auto func>
  void regist_one_handler();

 public:
  std::function<std::optional<std::string>(
      std::string_view, rpc_conn, rpc_protocol::req_header &req_head,
      typename rpc_protocol::supported_serialize_protocols protocols)>
      *get_handler(uint32_t id);

  std::function<async_simple::coro::Lazy<std::optional<std::string>>(
      std::string_view, rpc_conn, rpc_protocol::req_header &req_head,
      typename rpc_protocol::supported_serialize_protocols protocols)>
      *get_coro_handler(uint32_t id);

  async_simple::coro::Lazy<std::pair<std::errc, std::string>> route_coro(
      auto handler, std::string_view data, rpc_conn conn,
      rpc_protocol::req_header &header,
      typename rpc_protocol::supported_serialize_protocols protocols);

  std::pair<std::errc, std::string> route(
      auto handler, std::string_view data, rpc_conn conn,
      rpc_protocol::req_header &header,
      typename rpc_protocol::supported_serialize_protocols protocols);

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
  void regist_handler(class_type_t<decltype(first)> *self);

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
  void regist_handler();
};

}  // namespace protocol
}  // namespace coro_rpc