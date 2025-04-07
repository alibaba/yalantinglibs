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
#include <ylt/coro_http/coro_http_server.hpp>
#include <ylt/coro_rpc/coro_rpc_server.hpp>

#include "rpc_service.h"
using namespace coro_rpc;
using namespace async_simple;
using namespace async_simple::coro;
int main() {
  // init rpc server
  coro_rpc_server server(/*thread=*/std::thread::hardware_concurrency(),
                         /*port=*/8801);

  coro_rpc_server server2{/*thread=*/1, /*port=*/8802};

  // regist normal function for rpc
  server.register_handler<echo, async_echo_by_coroutine, async_echo_by_callback,
                          echo_with_attachment, nested_echo,
                          return_error_by_context, return_error_by_exception,
                          rpc_with_state_by_tag, get_ctx_info,
                          rpc_with_complete_handler, add>();

  // regist member function for rpc
  HelloService hello_service;
  server.register_handler<&HelloService::hello>(&hello_service);

  server2.register_handler<echo>();
  // async start server
  auto res = server2.async_start();
  assert(!res.hasResult());

  // start an http server in same port(8801) of rpc server. you can open
  // http://localhost:8801/ to visit it.
  auto http_server = std::make_unique<coro_http::coro_http_server>(0, 0);
  http_server->set_http_handler<coro_http::GET>(
      "/", [](coro_http::coro_http_request& req,
              coro_http::coro_http_response& resp) {
        resp.set_status_and_content(coro_http::status_type::ok,
                                    R"(<!DOCTYPE html>
<html>
    <head>
        <title>Example</title>
    </head>
    <body>
        <p>This is an example of a simple HTML page with one paragraph.</p>
    </body>
</html>)");
      });
  std::function dispatcher = [](coro_io::socket_wrapper_t&& soc,
                                std::string_view magic_number,
                                coro_http::coro_http_server& server) {
    server.transfer_connection(std::move(soc), magic_number);
  };
  server.add_subserver(std::move(dispatcher), std::move(http_server));

  // sync start server & sync await server stop
  return !server.start();
}