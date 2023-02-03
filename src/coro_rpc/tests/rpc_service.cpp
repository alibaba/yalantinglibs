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
#include <easylog/easylog.h>

#include <coro_rpc/rpc_connection.hpp>

#include "rpc_api.hpp"

using namespace coro_rpc;
using namespace std::chrono_literals;
using namespace std::string_literals;

void hi() { ELOGV(INFO, "call hi"); }
std::string hello() { return "hello"; }
std::string hello_timeout() {
  std::this_thread::sleep_for(40ms);
  return "hello";
}

std::string client_hello() { return "client hello"; }

std::string client_hello_not_reg() { return "client hello"; }

std::string large_arg_fun(std::string data) {
  ELOGV(INFO, "data size: %d", data.size());
  return data;
}

int long_run_func(int val) {
  std::this_thread::sleep_for(40ms);
  return val;
}

template <typename Conn>
void fun_with_delay_return_void(coro_rpc::connection<void, Conn> conn) {
  conn.response_msg();
}

template <typename Conn>
void fun_with_delay_return_string(
    coro_rpc::connection<std::string, Conn> conn) {
  conn.response_msg("string"s);
}

template <typename Conn>
void fun_with_delay_return_void_twice(coro_rpc::connection<void, Conn> conn) {
  conn.response_msg();
  conn.response_msg();
}

template <typename Conn>
void fun_with_delay_return_string_twice(
    coro_rpc::connection<std::string, Conn> conn) {
  conn.response_msg("string"s);
  conn.response_msg("string"s);
}
void coro_fun_with_delay_return_void(
    coro_rpc::connection<void, coro_connection> conn) {
  fun_with_delay_return_void(conn);
}


void coro_fun_with_delay_return_string(
    coro_rpc::connection<std::string, coro_connection> conn) {
  fun_with_delay_return_string(conn);
}


void coro_fun_with_delay_return_void_twice(
    coro_rpc::connection<void, coro_connection> conn) {
  fun_with_delay_return_void_twice(conn);
}


void coro_fun_with_delay_return_string_twice(
    coro_rpc::connection<std::string, coro_connection> conn) {
  fun_with_delay_return_string_twice(conn);
}


template <typename Conn>
void fun_with_delay_return_void_cost_long_time(
    coro_rpc::connection<void, Conn> conn) {
  std::thread([conn]() mutable {
    std::this_thread::sleep_for(400ms);
    conn.response_msg();
  }).detach();
}

void coro_fun_with_delay_return_void_cost_long_time(
    coro_rpc::connection<void, coro_rpc::coro_connection> conn) {
  fun_with_delay_return_void_cost_long_time(conn);
}

std::string async_hi() { return "async hi"; }

std::string HelloService::hello() {
  ELOGV(INFO, "call HelloServer hello");
  return "hello";
}
std::string HelloService::static_hello() {
  ELOGV(INFO, "call static hello");
  return "static hello";
}

namespace ns_login {
bool LoginService::login(std::string username, std::string password) {
  return true;
}
}  // namespace ns_login