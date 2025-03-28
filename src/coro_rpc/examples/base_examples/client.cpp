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
#include <string>
#include <vector>
#include <ylt/coro_rpc/coro_rpc_client.hpp>

#include "async_simple/Try.h"
#include "async_simple/coro/Collect.h"
#include "async_simple/coro/Lazy.h"
#include "rpc_service.h"
#include "ylt/coro_rpc/impl/coro_rpc_client.hpp"
#include "ylt/coro_rpc/impl/errno.h"
#include "ylt/coro_rpc/impl/protocol/coro_rpc_protocol.hpp"
using namespace coro_rpc;
using namespace async_simple::coro;
using namespace std::string_literals;
/*!
 * \example helloworld/client.main.cpp
 * \brief helloworld example client code
 */

Lazy<void> show_rpc_call() {
  coro_rpc_client client;

  [[maybe_unused]] auto ec = co_await client.connect("127.0.0.1", "8801");
  assert(!ec);

  /*----------------rdma---------------*/
  resources resource{};
  resources_create(&resource);
  asio::posix::stream_descriptor cq_fd(
      client.get_executor().get_asio_executor(),
      resource.complete_event_channel->fd);
  int r = ibv_req_notify_cq(resource.cq, 0);
  assert(r >= 0);

  resources* res = &resource;

  cm_con_data_t local_data{};
  local_data.addr = (uintptr_t)res->buf;
  local_data.rkey = res->mr->rkey;
  local_data.qp_num = res->qp->qp_num;
  local_data.lid = res->port_attr.lid;
  union ibv_gid my_gid;
  memset(&my_gid, 0, sizeof(my_gid));

  if (config.gid_idx >= 0) {
    CHECK(ibv_query_gid(res->ib_ctx, config.ib_port, config.gid_idx, &my_gid));
  }

  memcpy(local_data.gid, &my_gid, 16);

  auto rdma_ret =
      co_await client.call<&rdma_service_t::get_con_data>(local_data);
  g_remote_con_data = rdma_ret.value();
  connect_qp(res);

  for (int i = 0; i < 3; i++) {
    auto rdma_ret1 = co_await client.call<&rdma_service_t::fetch>();
    // poll_completion(res);
    coro_io::callback_awaitor<std::error_code> awaitor;
    auto ec = co_await awaitor.await_resume([&cq_fd](auto handler) {
      cq_fd.async_wait(asio::posix::stream_descriptor::wait_read,
                       [handler](const auto& ec) mutable {
                         handler.set_value_then_resume(ec);
                       });
    });

    poll_completion(res);

    std::cout << std::string_view(res->buf) << "\n";
    post_receive(res);
  }

  for (int i = 0; i < 3; i++) {
    std::string msg = "hello rdma from client ";
    msg.append(std::to_string(i));
    post_send(res, IBV_WR_SEND, msg);
    coro_io::callback_awaitor<std::error_code> awaitor;
    auto ec = co_await awaitor.await_resume([&cq_fd](auto handler) {
      cq_fd.async_wait(asio::posix::stream_descriptor::wait_read,
                       [handler](const auto& ec) mutable {
                         handler.set_value_then_resume(ec);
                       });
    });
    poll_completion(res);
    co_await client.call<&rdma_service_t::put>();
  }
  resources_destroy(res);
  co_return;
  /*----------------rdma---------------*/

  auto ret = co_await client.call<echo>("hello");
  assert(ret.value() == "hello");

  ret = co_await client.call<async_echo_by_coroutine>("42");
  assert(ret.value() == "42");

  ret = co_await client.call<async_echo_by_callback>("hi");
  assert(ret.value() == "hi");

  client.set_req_attachment("This is attachment.");
  auto ret_void = co_await client.call<echo_with_attachment>();
  assert(client.get_resp_attachment() == "This is attachment.");

  client.set_req_attachment("This is attachment.");
  char buf[100];
  client.set_resp_attachment_buf(buf);
  ret_void = co_await client.call<echo_with_attachment>();
  assert(client.get_resp_attachment() == "This is attachment.");
  assert(client.is_resp_attachment_in_external_buf());
  assert(client.get_resp_attachment().data() == buf);

  ret = co_await client.call<nested_echo>("nested_echo"s);
  assert(ret.value() == "nested_echo"s);

  ret = co_await client.call<&HelloService::hello>();
  assert(ret.value() == "HelloService::hello"s);

  ret_void = co_await client.call<get_ctx_info>();
  assert(ret_void);

  ret_void = co_await client.call<return_error_by_context>();

  assert(ret_void.error().code.val() == 404);
  assert(ret_void.error().msg == "404 Not Found.");

  ret_void = co_await client.call<return_error_by_exception>();

  assert(ret_void.error().code.val() == 404);

  auto ret2 = co_await client.call<rpc_with_state_by_tag>();
  auto str = ret2.value();
  std::cout << ret2.value() << std::endl;
  assert(ret2.value() == "1");
  ret2 = co_await client.call<rpc_with_state_by_tag>();
  std::cout << ret2.value() << std::endl;
  assert(ret2.value() == "2");
  ret2 = co_await client.call<rpc_with_state_by_tag>();
  std::cout << ret2.value() << std::endl;
  assert(ret2.value() == "3");

  ret = co_await client.call<rpc_with_complete_handler>();
  assert(ret == "Hello");
}
/*send multi request with same socket in the same time*/
Lazy<void> connection_reuse() {
  coro_rpc_client client;
  [[maybe_unused]] auto ec = co_await client.connect("127.0.0.1", "8801");
  assert(!ec);
  std::vector<Lazy<async_rpc_result<int>>> handlers;
  for (int i = 0; i < 10; ++i) {
    /* send_request is thread-safe, so you can call it in different thread with
     * same client*/
    handlers.push_back(co_await client.send_request<add>(i, i + 1));
  }
  std::vector<async_simple::Try<async_rpc_result<int>>> results =
      co_await collectAll(std::move(handlers));
  for (int i = 0; i < 10; ++i) {
    std::cout << results[i].value()->result() << std::endl;
    assert(results[i].value()->result() == 2 * i + 1);
  }
  co_return;
}

int main() {
  try {
    syncAwait(show_rpc_call());
    syncAwait(connection_reuse());
    std::cout << "Done!" << std::endl;
  } catch (const std::exception& e) {
    std::cout << "Error:" << e.what() << std::endl;
  }
  return 0;
}