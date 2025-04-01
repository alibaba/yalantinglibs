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
#include <ylt/coro_rpc/coro_rpc_server.hpp>

#include "cmdline.h"
#include "rpc_service.h"
#include "ylt/coro_io/io_context_pool.hpp"
using namespace coro_rpc;
using namespace async_simple;
using namespace async_simple::coro;

int main(int argc, char** argv) {
#if NDEBUG
  easylog::set_min_severity(easylog::Severity::INFO);
#endif

  cmdline::parser parser;
  parser.add<unsigned short>("port", 'p', "server port", false, 8801);
  parser.add<uint32_t>("thd_num", 't', "server thread number", false,
                       std::thread::hardware_concurrency());
  parser.add<size_t>("reg_mr_len", 'r', "register memory lenght", false, 256);

  parser.parse_check(argc, argv);

  auto port = parser.get<unsigned short>("port");
  auto thd_num = parser.get<uint32_t>("thd_num");
  auto reg_mr_length = parser.get<size_t>("reg_mr_len");

  ELOG_INFO << "port: " << port << ", thd_num: " << thd_num
            << ", register mr lenght: " << reg_mr_length;

  // init rpc server
  coro_rpc_server server(thd_num, port);

  resources res{};
  resources_create(&res);

  auto executor = coro_io::get_global_executor();
  rdma_service_t service{
      &res,
      asio::posix::stream_descriptor(executor->get_asio_executor(),
                                     res.complete_event_channel->fd),
      executor};
  int r = ibv_req_notify_cq(res.cq, 0);
  assert(r >= 0);

  memset(&service.my_gid, 0, sizeof(service.my_gid));

  if (config.gid_idx >= 0) {
    CHECK(ibv_query_gid(res.ib_ctx, config.ib_port, config.gid_idx,
                        &service.my_gid));
  }
  service.reg_mr_len_ = reg_mr_length;

  server.register_handler<&rdma_service_t::get_con_data>(&service);

#if NDEBUG
  ELOG_INFO << "begin to statistic";
  int64_t last = 0;
  int64_t last_latence = 0;
  std::thread thd([&service, &last, &last_latence] {
    while (true) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      auto value = service.qps_.value();
      if (value == 0) {
        continue;
      }
      auto lat = service.latency_.value();
      auto qps = value - last;
      if (qps == 0) {
        continue;
      }
      ELOG_INFO << "qps: " << qps << ", latency: " << (lat - last_latence) / qps
                << "us";
      last = value;
      last_latence = lat;
    }
  });
#endif
  // regist normal function for rpc
  server.register_handler<echo, async_echo_by_coroutine, async_echo_by_callback,
                          echo_with_attachment, nested_echo,
                          return_error_by_context, return_error_by_exception,
                          rpc_with_state_by_tag, get_ctx_info,
                          rpc_with_complete_handler, add>();

  // regist member function for rpc
  HelloService hello_service;
  server.register_handler<&HelloService::hello>(&hello_service);

  // sync start server & sync await server stop
  return !server.start();
}