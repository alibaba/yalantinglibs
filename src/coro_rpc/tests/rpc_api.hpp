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
#ifndef CORO_RPC_RPC_API_HPP
#define CORO_RPC_RPC_API_HPP
#include <string>
#include <thread>
#include <ylt/coro_rpc/coro_rpc_context.hpp>

#include "ylt/coro_rpc/impl/errno.h"

void hi();
std::string hello();
std::string hello_timeout();
std::string client_hello();
std::string client_hello_not_reg();
std::string large_arg_fun(std::string data);
void function_not_registered();
int long_run_func(int val);
std::string async_hi();
struct my_context {
  coro_rpc::context<void> ctx_;
  my_context(coro_rpc::context<void>&& ctx) : ctx_(std::move(ctx)) {}
  using return_type = void;
};
void echo_with_attachment(coro_rpc::context<void> conn);
inline void error_with_context(coro_rpc::context<void> conn) {
  conn.response_error(coro_rpc::err_code{104}, "My Error.");
}
void coro_fun_with_user_define_connection_type(my_context conn);
void coro_fun_with_delay_return_void(coro_rpc::context<void> conn);
void coro_fun_with_delay_return_string(coro_rpc::context<std::string> conn);
void coro_fun_with_delay_return_void_twice(coro_rpc::context<void> conn);
void coro_fun_with_delay_return_string_twice(
    coro_rpc::context<std::string> conn);
void coro_fun_with_delay_return_void_cost_long_time(
    coro_rpc::context<void> conn);
inline async_simple::coro::Lazy<void> coro_func_return_void(int i) {
  co_return;
}
inline async_simple::coro::Lazy<int> coro_func(int i) { co_return i; }
inline async_simple::coro::Lazy<void> coro_func_delay_return_int(
    coro_rpc::context<int> conn, int i) {
  conn.response_msg(i);
  co_return;
}
inline async_simple::coro::Lazy<void> coro_func_delay_return_void(
    coro_rpc::context<void> conn, int i) {
  conn.response_msg();
  co_return;
}

class HelloService {
 public:
  std::string hello();
  static std::string static_hello();
  async_simple::coro::Lazy<int> coro_func(int i) { co_return i; }
  async_simple::coro::Lazy<void> coro_func_return_void(int i) { co_return; }
  async_simple::coro::Lazy<void> coro_func_delay_return_int(
      coro_rpc::context<int> conn, int i) {
    conn.response_msg(i);
    co_return;
  }
  async_simple::coro::Lazy<void> coro_func_delay_return_void(
      coro_rpc::context<void> conn, int i) {
    conn.response_msg();
    co_return;
  }

 private:
};
namespace ns_login {
class LoginService {
 public:
  bool login(std::string username, std::string password);
};
}  // namespace ns_login
#endif  // CORO_RPC_RPC_API_HPP
