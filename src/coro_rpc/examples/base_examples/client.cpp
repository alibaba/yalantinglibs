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
#include <chrono>
#include <string>
#include <vector>
#include <ylt/coro_rpc/coro_rpc_client.hpp>

#include "async_simple/Try.h"
#include "async_simple/coro/Collect.h"
#include "async_simple/coro/Lazy.h"
#include "rpc_service.h"
#include "ylt/coro_io/coro_io.hpp"
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
  auto qp = create_qp(res->pd, res->cq);
  auto mr = create_mr(res->pd);
  auto ctx = std::make_shared<conn_context>(conn_context{mr, qp});

  cm_con_data_t local_data{};
  local_data.addr = (uintptr_t)mr->addr;
  local_data.rkey = mr->rkey;
  local_data.qp_num = qp->qp_num;
  local_data.lid = res->port_attr.lid;
  union ibv_gid my_gid;
  memset(&my_gid, 0, sizeof(my_gid));

  if (config.gid_idx >= 0) {
    CHECK(ibv_query_gid(res->ib_ctx, config.ib_port, config.gid_idx, &my_gid));
  }

  memcpy(local_data.gid, &my_gid, 16);

  auto rdma_ret = co_await client.call_for<&rdma_service_t::get_con_data>(
      std::chrono::seconds(3600), local_data);
  // g_remote_con_data = rdma_ret.value();
  connect_qp(qp, rdma_ret.value());

  async_simple::Promise<void> promise;
  auto on_response =
      [res, &cq_fd, &promise](
          std::shared_ptr<conn_context> ctx) -> async_simple::coro::Lazy<void> {
    auto sp = ctx;
    while (true) {
      coro_io::callback_awaitor<std::error_code> awaitor;
      auto ec = co_await awaitor.await_resume([&cq_fd](auto handler) {
        cq_fd.async_wait(asio::posix::stream_descriptor::wait_read,
                         [handler](const auto& ec) mutable {
                           handler.set_value_then_resume(ec);
                         });
      });
      if (ec) {
        ELOG_INFO << ec.message();
        if (ctx->recv_id)
          resume<int>(ec.value(), ctx->recv_id);

        if (ctx->send_id)
          resume<int>(ec.value(), ctx->send_id);

        promise.setValue();
        break;
      }

      poll_completion(res, ctx);
    }
  };

  on_response(ctx).via(&client.get_executor()).start([](auto&&) {
  });

  auto close_lz = [&cq_fd]() -> async_simple::coro::Lazy<int> {
    std::error_code ignore;
    cq_fd.cancel(ignore);
    cq_fd.close(ignore);
    co_return 0;
  };

  for (int i = 0; i < 3; i++) {
    // send request to server
    std::string msg = "hello rdma from client ";
    msg.append(std::to_string(i));

    // auto [rr, sr, cr] = co_await async_simple::coro::collectAll(
    //     post_receive_coro(res), post_send_coro(res, IBV_WR_SEND, msg),
    //     close_lz()); // for timeout test
    auto [rr, sr] = co_await async_simple::coro::collectAll(
        post_receive_coro(ctx.get()), post_send_coro(ctx.get(), msg));
    if (rr.value() || sr.value()) {
      ELOG_ERROR << "rdma send recv error";
      break;
    }
  }

  std::error_code ignore;
  cq_fd.cancel(ignore);
  cq_fd.close(ignore);

  co_await promise.getFuture();

  ibv_destroy_qp(ctx->qp);
  free(ctx->mr->addr);
  ibv_dereg_mr(ctx->mr);
  resources_destroy(res);

  co_return;
  /*----------------rdma---------------*/
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
    // syncAwait(connection_reuse());
    std::cout << "Done!" << std::endl;
  } catch (const std::exception& e) {
    std::cout << "Error:" << e.what() << std::endl;
  }
  return 0;
}