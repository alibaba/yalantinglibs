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
#ifndef CORO_RPC_RPC_API_HPP
#define CORO_RPC_RPC_API_HPP
#include <coro_rpc/rpc_connection.hpp>
#include <string>
#include <thread>
void hi();
std::string hello();
std::string hello_timeout();
std::string client_hello();
std::string client_hello_not_reg();
std::string large_arg_fun(std::string data);
int long_run_func(int val);
void timeout_due_to_heartbeat();
std::string async_hi();
void coro_fun_with_delay_return_void(
    coro_rpc::connection<void, coro_rpc::coro_connection> conn);
void async_fun_with_delay_return_void(
    coro_rpc::connection<void, coro_rpc::async_connection> conn);
void coro_fun_with_delay_return_string(
    coro_rpc::connection<std::string, coro_rpc::coro_connection> conn);
void async_fun_with_delay_return_string(
    coro_rpc::connection<std::string, coro_rpc::async_connection> conn);
void coro_fun_with_delay_return_void_twice(
    coro_rpc::connection<void, coro_rpc::coro_connection> conn);
void async_fun_with_delay_return_void_twice(
    coro_rpc::connection<void, coro_rpc::async_connection> conn);
void coro_fun_with_delay_return_string_twice(
    coro_rpc::connection<std::string, coro_rpc::coro_connection> conn);
void async_fun_with_delay_return_string_twice(
    coro_rpc::connection<std::string, coro_rpc::async_connection> conn);
void coro_fun_with_delay_return_void_cost_long_time(
    coro_rpc::connection<void, coro_rpc::coro_connection> conn);
void async_fun_with_delay_return_void_cost_long_time(
    coro_rpc::connection<void, coro_rpc::async_connection> conn);
inline async_simple::coro::Lazy<int> coro_func(int i) { co_return i; }
class HelloService {
 public:
  std::string hello();
  static std::string static_hello();
  async_simple::coro::Lazy<int> coro_func(int i) { co_return i; }

 private:
};
namespace ns_login {
class LoginService {
 public:
  bool login(std::string username, std::string password);
};
}  // namespace ns_login
#endif  // CORO_RPC_RPC_API_HPP
