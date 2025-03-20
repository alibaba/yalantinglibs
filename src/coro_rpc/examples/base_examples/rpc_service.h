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

#include <arpa/inet.h>
#include <infiniband/verbs.h>

#include <sstream>
#include <string>
#include <string_view>
#include <ylt/coro_rpc/coro_rpc_context.hpp>

struct pingpong_context {
  struct ibv_context *context;
  struct ibv_pd *pd;
  struct ibv_mr *mr;
  struct ibv_cq *cq;
  struct ibv_qp *qp;

  char *buf;
  uint32_t size;
  int lid;
  std::string str_gid;

  ~pingpong_context() {
    ibv_destroy_qp(qp);
    ibv_destroy_cq(cq);
    ibv_dereg_mr(mr);
    free(buf);
    ibv_dealloc_pd(pd);
    ibv_close_device(context);
  }
};

struct pingpong_dest {
  int lid;
  int qpn;
  int psn;
  std::string gid;
};

inline void wire_gid_to_gid(const char *wgid, union ibv_gid *gid) {
  char tmp[9];
  __be32 v32;
  int i;
  uint32_t tmp_gid[4];

  for (tmp[8] = 0, i = 0; i < 4; ++i) {
    memcpy(tmp, wgid + i * 8, 8);
    sscanf(tmp, "%x", &v32);
    tmp_gid[i] = be32toh(v32);
  }
  memcpy(gid, tmp_gid, sizeof(*gid));
}

inline std::string get_gid_str(union ibv_gid gid) {
  std::string gid_str;
  char buf[16] = {0};
  const static size_t kGidLength = 16;
  for (size_t i = 0; i < kGidLength; ++i) {
    sprintf(buf, "%02x", gid.raw[i]);
    gid_str += i == 0 ? buf : std::string(":") + buf;
  }

  return gid_str;
}

inline pingpong_context create_rdma_ctx() {
  pingpong_context pp_ctx{};
  struct ibv_device **dev_list = ibv_get_device_list(nullptr);
  assert(dev_list);
  pp_ctx.context = ibv_open_device(dev_list[0]);
  ibv_free_device_list(dev_list);

  pp_ctx.pd = ibv_alloc_pd(pp_ctx.context);
  assert(pp_ctx.pd);
  int BUF_SIZE = 256;
  pp_ctx.buf = (char *)malloc(BUF_SIZE);
  pp_ctx.size = BUF_SIZE;
  pp_ctx.mr = ibv_reg_mr(pp_ctx.pd, pp_ctx.buf, BUF_SIZE,
                         IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ |
                             IBV_ACCESS_REMOTE_WRITE);
  assert(pp_ctx.mr);

  pp_ctx.cq = ibv_create_cq(pp_ctx.context, 10, nullptr, nullptr, 0);
  assert(pp_ctx.cq);

  struct ibv_qp_init_attr qp_init_attr = {
      .send_cq = pp_ctx.cq,
      .recv_cq = pp_ctx.cq,
      .cap =
          {
              .max_send_wr = 10,
              .max_recv_wr = 10,
              .max_send_sge = 1,
              .max_recv_sge = 1,
          },
      .qp_type = IBV_QPT_RC,
  };
  pp_ctx.qp = ibv_create_qp(pp_ctx.pd, &qp_init_attr);
  assert(pp_ctx.qp);
  int ib_port = 1;
  struct ibv_qp_attr attr {};
  attr.qp_state = IBV_QPS_INIT;
  attr.pkey_index = 0;
  attr.port_num = ib_port;
  attr.qp_access_flags = IBV_ACCESS_REMOTE_WRITE | IBV_ACCESS_REMOTE_READ |
                         IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_ATOMIC;

  if (ibv_modify_qp(pp_ctx.qp, &attr,
                    IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT |
                        IBV_QP_ACCESS_FLAGS)) {
    fprintf(stderr, "Failed to modify QP to INIT\n");
  }

  struct ibv_port_attr portinfo;
  if (ibv_query_port(pp_ctx.context, ib_port, &portinfo)) {
    std::cout << "can not get port info\n";
  }

  pp_ctx.lid = portinfo.lid;

  union ibv_gid gid {};
  int gid_index = 0;
  if (ibv_query_gid(pp_ctx.context, ib_port, gid_index, &gid)) {
    fprintf(stderr, "can't read sgid of index %d\n", gid_index);
  }

  auto str = get_gid_str(gid);

  pp_ctx.str_gid = str;

  // inet_ntop(AF_INET6, &gid, pp_ctx.str_gid, sizeof pp_ctx.str_gid);

  ELOG_INFO << "local gid: " << pp_ctx.str_gid
            << ", qp_num: " << pp_ctx.qp->qp_num;

  return pp_ctx;
}

inline void post_recv(pingpong_context &pp_ctx) {
  struct ibv_sge sge {};
  sge.addr = reinterpret_cast<uint64_t>(pp_ctx.mr->addr);
  sge.length = pp_ctx.mr->length;
  sge.lkey = pp_ctx.mr->lkey;

  struct ibv_recv_wr recv_wr {};
  recv_wr.next = nullptr;
  recv_wr.num_sge = 1;
  recv_wr.wr_id = 1;
  recv_wr.sg_list = &sge;

  struct ibv_recv_wr *bad_wr;
  if (ibv_post_recv(pp_ctx.qp, &recv_wr, &bad_wr)) {
    std::cout << "post recv failed\n";
  }
}

inline void post_send(pingpong_context &pp_ctx, std::string msg) {
  memcpy(pp_ctx.buf, msg.data(), msg.size());

  struct ibv_sge send_sge {};
  send_sge.addr = reinterpret_cast<uint64_t>(pp_ctx.mr->addr);
  send_sge.length = pp_ctx.mr->length;
  send_sge.lkey = pp_ctx.mr->lkey;

  struct ibv_send_wr send_wr = {};
  struct ibv_send_wr *bad_send_wr = nullptr;
  send_wr.opcode = IBV_WR_SEND;
  send_wr.next = nullptr;
  send_wr.num_sge = 1;
  send_wr.wr_id = 2;
  send_wr.send_flags = IBV_SEND_SIGNALED;
  send_wr.sg_list = &send_sge;

  struct ibv_send_wr *bad_wr;
  if (int r = ibv_post_send(pp_ctx.qp, &send_wr, &bad_wr)) {
    std::cout << "post send failed " << r << "\n";
  }
}

inline std::atomic<int> g_psn = 0;

inline int modify_qp_to_rts(struct ibv_qp *qp, pingpong_dest dest,
                            bool is_server = true) {
  union ibv_gid gid {};
  std::istringstream iss(dest.gid);
  for (int i = 0; i < 16; ++i) {
    int value;
    iss >> std::hex >> value;
    gid.raw[i] = static_cast<uint8_t>(value);
    if (i < 15)
      iss.ignore(1, ':');
  }

  struct ibv_qp_attr qp_attr = {};
  ::bzero(&qp_attr, sizeof(qp_attr));
  qp_attr.qp_state = IBV_QPS_RTR;
  qp_attr.path_mtu = IBV_MTU_4096;
  qp_attr.dest_qp_num = dest.qpn;
  qp_attr.rq_psn = dest.psn;
  qp_attr.max_dest_rd_atomic = 16;
  qp_attr.min_rnr_timer = 12;
  qp_attr.ah_attr.is_global = 1;
  qp_attr.ah_attr.grh.dgid = gid;
  qp_attr.ah_attr.grh.sgid_index = 0;
  qp_attr.ah_attr.grh.hop_limit = 16;
  qp_attr.ah_attr.dlid = dest.lid;
  qp_attr.ah_attr.sl = 0;
  qp_attr.ah_attr.src_path_bits = 0;
  qp_attr.ah_attr.port_num = 1;

  if (int r =
          ibv_modify_qp(qp, &qp_attr,
                        IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU |
                            IBV_QP_DEST_QPN | IBV_QP_RQ_PSN |
                            IBV_QP_MIN_RNR_TIMER | IBV_QP_MAX_DEST_RD_ATOMIC)) {
    std::cout << "Failed to modify QP to RTR " << r << "\n";
  }

  if (is_server) {
    qp_attr.qp_state = IBV_QPS_RTS;
    qp_attr.timeout = 14;
    qp_attr.retry_cnt = 7;
    qp_attr.rnr_retry = 7;
    qp_attr.sq_psn = g_psn++;
    qp_attr.max_rd_atomic = 16;

    if (int r = ibv_modify_qp(qp, &qp_attr,
                              IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT |
                                  IBV_QP_RNR_RETRY | IBV_QP_SQ_PSN |
                                  IBV_QP_MAX_QP_RD_ATOMIC)) {
      std::cout << "Failed to modify QP to RTS " << r << "\n";
      return 1;
    }
  }
  return 0;
}

struct rdma_service_t {
  pingpong_context ctx;
  pingpong_dest peer;
  pingpong_dest handshake(pingpong_dest from_peer) {
    ELOG_INFO << "remote gid: " << from_peer.gid
              << ", remote qp_num: " << from_peer.qpn;
    peer = from_peer;

    modify_qp_to_rts(ctx.qp, from_peer);

    pingpong_dest dest{};
    dest.lid = ctx.lid;
    dest.qpn = ctx.qp->qp_num;
    dest.psn = g_psn++;
    dest.gid = ctx.str_gid;

    return dest;
  }

  void test() {
    post_send(ctx, "hello rdma!");
    post_send(ctx, "hello rdma!");
  }
};

std::string_view echo(std::string_view data);
async_simple::coro::Lazy<std::string_view> async_echo_by_coroutine(
    std::string_view data);
void async_echo_by_callback(
    coro_rpc::context<std::string_view /*rpc response data here*/> conn,
    std::string_view /*rpc request data here*/ data);
void echo_with_attachment();
inline int add(int a, int b) { return a + b; }
async_simple::coro::Lazy<std::string_view> nested_echo(std::string_view sv);
void return_error_by_context(coro_rpc::context<void> conn);
void return_error_by_exception();
async_simple::coro::Lazy<void> get_ctx_info();
class HelloService {
 public:
  std::string_view hello();
};
async_simple::coro::Lazy<std::string> rpc_with_state_by_tag();
std::string_view rpc_with_complete_handler();
#endif  // CORO_RPC_RPC_API_HPP
