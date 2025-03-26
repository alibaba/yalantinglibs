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
#include <sys/epoll.h>

#include <asio/posix/stream_descriptor.hpp>
#include <string>
#include <string_view>
#include <ylt/coro_rpc/coro_rpc_context.hpp>

/*-------------- rdma ---------------*/
struct cm_con_data_t {
  uint64_t addr;    // buffer address
  uint32_t rkey;    // remote key
  uint32_t qp_num;  // QP number
  uint16_t lid;     // LID of the IB port
  uint8_t gid[16];  // GID
};

struct resources {
  struct ibv_device_attr device_attr;  // device attributes
  struct ibv_port_attr port_attr;      // IB port attributes
  struct cm_con_data_t remote_props;   // values to connect to remote side
  struct ibv_context *ib_ctx;          // device handle
  struct ibv_pd *pd;                   // PD handle
  struct ibv_comp_channel *complete_event_channel;
  struct ibv_cq *cq;  // CQ handle
  struct ibv_qp *qp;  // QP handle
  struct ibv_mr *mr;  // MR handle for buf
  char *buf;          // memory buffer pointer, used for
                      // RDMA send ops
};

#define CHECK(expr)            \
  {                            \
    int rc = (expr);           \
    if (rc != 0) {             \
      perror(strerror(errno)); \
      exit(EXIT_FAILURE);      \
    }                          \
  }

struct config_t {
  char *dev_name = NULL;
  uint16_t tcp_port = 20000;
  uint16_t ib_port = 1;
  int gid_idx = 0;
};

inline config_t config{};
inline size_t g_buf_size = 256;

inline int resources_create(resources *res) {
  struct ibv_device **dev_list = NULL;
  struct ibv_qp_init_attr qp_init_attr;
  struct ibv_device *ib_dev = NULL;

  size_t size;
  int i;
  int mr_flags = 0;
  int cq_size = 0;
  int num_devices;

  ELOGV(INFO, "Searching for IB devices in host");

  // \begin acquire a specific device
  // get device names in the system
  dev_list = ibv_get_device_list(&num_devices);
  assert(dev_list != NULL);

  if (num_devices == 0) {
    ELOGV(ERROR, "Found %d device(s)", num_devices);
    exit(EXIT_FAILURE);
  }

  ELOGV(INFO, "Found %d device(s)", num_devices);

  // search for the specific device we want to work with
  for (i = 0; i < num_devices; i++) {
    if (!config.dev_name) {
      config.dev_name = strdup(ibv_get_device_name(dev_list[i]));
      ELOGV(INFO, "Device not specified, using first one found: %s",
            config.dev_name);
    }

    if (strcmp(ibv_get_device_name(dev_list[i]), config.dev_name) == 0) {
      ib_dev = dev_list[i];
      break;
    }
  }

  // device wasn't found in the host
  if (!ib_dev) {
    ELOGV(ERROR, "IB device %s wasn't found", config.dev_name);
    exit(EXIT_FAILURE);
  }

  // get device handle
  res->ib_ctx = ibv_open_device(ib_dev);
  assert(res->ib_ctx != NULL);
  // \end acquire a specific device

  // query port properties
  CHECK(ibv_query_port(res->ib_ctx, config.ib_port, &res->port_attr));

  // PD
  res->pd = ibv_alloc_pd(res->ib_ctx);
  assert(res->pd != NULL);

  // a CQ with 10 entry
  cq_size = 10;

  res->complete_event_channel = ibv_create_comp_channel(res->ib_ctx);
  res->cq =
      ibv_create_cq(res->ib_ctx, cq_size, NULL, res->complete_event_channel, 0);
  assert(res->cq != NULL);

  // set non blocking
  int flags = ::fcntl(res->complete_event_channel->fd, F_GETFL);
  assert(flags >= 0);
  int ret =
      ::fcntl(res->complete_event_channel->fd, F_SETFL, flags | O_NONBLOCK);
  assert(ret >= 0);

  // epoll
  int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
  epoll_event ev = {0, {0}};
  ev.events = EPOLLIN | EPOLLET;
  // descriptor_data->registered_events_ = ev.events;
  // ev.data.ptr = descriptor_data;
  int result =
      epoll_ctl(epoll_fd, EPOLL_CTL_ADD, res->complete_event_channel->fd, &ev);
  assert(result == 0);

  int r = ibv_req_notify_cq(res->cq, 0);
  assert(r >= 0);

  // a buffer to hold the data
  size = g_buf_size;
  res->buf = (char *)calloc(1, g_buf_size);
  assert(res->buf != NULL);

  // register the memory buffer
  mr_flags =
      IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;
  res->mr = ibv_reg_mr(res->pd, res->buf, size, mr_flags);
  assert(res->mr != NULL);

  ELOGV(INFO, "MR was registered with addr=%p, lkey= %d, rkey= %d, flags= %d",
        res->buf, res->mr->lkey, res->mr->rkey, mr_flags);

  // \begin create the QP
  memset(&qp_init_attr, 0, sizeof(qp_init_attr));
  qp_init_attr.qp_type = IBV_QPT_RC;
  qp_init_attr.sq_sig_all = 1;
  qp_init_attr.send_cq = res->cq;
  qp_init_attr.recv_cq = res->cq;
  qp_init_attr.cap.max_send_wr = 10;
  qp_init_attr.cap.max_recv_wr = 10;
  qp_init_attr.cap.max_send_sge = 1;
  qp_init_attr.cap.max_recv_sge = 1;

  res->qp = ibv_create_qp(res->pd, &qp_init_attr);
  assert(res->qp != NULL);

  ELOGV(INFO, "QP was created, QP number= %d", res->qp->qp_num);
  // \end create the QP

  return 0;
}

// Transition a QP from the RESET to INIT state
inline int modify_qp_to_init(struct ibv_qp *qp) {
  struct ibv_qp_attr attr;
  int flags;

  memset(&attr, 0, sizeof(attr));
  attr.qp_state = IBV_QPS_INIT;
  attr.port_num = config.ib_port;
  attr.pkey_index = 0;
  attr.qp_access_flags =
      IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE;

  flags = IBV_QP_STATE | IBV_QP_PKEY_INDEX | IBV_QP_PORT | IBV_QP_ACCESS_FLAGS;

  CHECK(ibv_modify_qp(qp, &attr, flags));

  ELOGV(INFO, "Modify QP to INIT done!");

  // FIXME: ;)
  return 0;
}

// Transition a QP from the INIT to RTR state, using the specified QP number
inline int modify_qp_to_rtr(struct ibv_qp *qp, uint32_t remote_qpn,
                            uint16_t dlid, uint8_t *dgid) {
  struct ibv_qp_attr attr;
  int flags;

  memset(&attr, 0, sizeof(attr));

  attr.qp_state = IBV_QPS_RTR;
  attr.path_mtu = IBV_MTU_256;
  attr.dest_qp_num = remote_qpn;
  attr.rq_psn = 0;
  attr.max_dest_rd_atomic = 1;
  attr.min_rnr_timer = 0x12;
  attr.ah_attr.is_global = 0;
  attr.ah_attr.dlid = dlid;
  attr.ah_attr.sl = 0;
  attr.ah_attr.src_path_bits = 0;
  attr.ah_attr.port_num = config.ib_port;

  if (config.gid_idx >= 0) {
    attr.ah_attr.is_global = 1;
    attr.ah_attr.port_num = 1;
    memcpy(&attr.ah_attr.grh.dgid, dgid, 16);
    attr.ah_attr.grh.flow_label = 0;
    attr.ah_attr.grh.hop_limit = 1;
    attr.ah_attr.grh.sgid_index = config.gid_idx;
    attr.ah_attr.grh.traffic_class = 0;
  }

  flags = IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU | IBV_QP_DEST_QPN |
          IBV_QP_RQ_PSN | IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER;

  CHECK(ibv_modify_qp(qp, &attr, flags));

  ELOGV(INFO, "Modify QP to RTR done!");

  return 0;
}

// Transition a QP from the RTR to RTS state
static int modify_qp_to_rts(struct ibv_qp *qp) {
  struct ibv_qp_attr attr;
  int flags;

  memset(&attr, 0, sizeof(attr));

  attr.qp_state = IBV_QPS_RTS;
  attr.timeout = 0x12;  // 18
  attr.retry_cnt = 6;
  attr.rnr_retry = 0;
  attr.sq_psn = 0;
  attr.max_rd_atomic = 1;

  flags = IBV_QP_STATE | IBV_QP_TIMEOUT | IBV_QP_RETRY_CNT | IBV_QP_RNR_RETRY |
          IBV_QP_SQ_PSN | IBV_QP_MAX_QP_RD_ATOMIC;

  CHECK(ibv_modify_qp(qp, &attr, flags));

  ELOGV(INFO, "Modify QP to RTS done!");

  return 0;
}

inline int post_receive(resources *res) {
  struct ibv_recv_wr rr;
  struct ibv_sge sge;
  struct ibv_recv_wr *bad_wr = nullptr;

  // prepare the scatter / gather entry
  memset(&sge, 0, sizeof(sge));
  sge.addr = (uintptr_t)res->buf;
  sge.length = g_buf_size;
  sge.lkey = res->mr->lkey;

  // prepare the receive work request
  memset(&rr, 0, sizeof(rr));

  rr.next = NULL;
  rr.wr_id = 0;
  rr.sg_list = &sge;
  rr.num_sge = 1;

  // post the receive request to the RQ
  CHECK(ibv_post_recv(res->qp, &rr, &bad_wr));
  ELOGV(INFO, "Receive request was posted");

  return 0;
}

inline cm_con_data_t g_remote_con_data{};

inline int connect_qp(resources *res) {
  struct cm_con_data_t local_con_data;
  char temp_char;
  union ibv_gid my_gid;

  memset(&my_gid, 0, sizeof(my_gid));

  if (config.gid_idx >= 0) {
    CHECK(ibv_query_gid(res->ib_ctx, config.ib_port, config.gid_idx, &my_gid));
  }

  // save the remote side attributes, we will need it for the post SR
  res->remote_props = g_remote_con_data;
  memcpy(res->remote_props.gid, g_remote_con_data.gid, 16);
  // \end exchange required info

  ELOGV(INFO, "Remote address = 0x%x", g_remote_con_data.addr);
  ELOGV(INFO, "Remote rkey = %d", g_remote_con_data.rkey);
  ELOGV(INFO, "Remote QP number = %d", g_remote_con_data.qp_num);
  ELOGV(INFO, "Remote LID = %d", g_remote_con_data.lid);

  if (config.gid_idx >= 0) {
    uint8_t *p = g_remote_con_data.gid;
    int i;
    printf("Remote GID = ");
    for (i = 0; i < 15; i++) printf("%02x:", p[i]);
    printf("%02x\n", p[15]);
  }

  // modify the QP to init
  modify_qp_to_init(res->qp);

  post_receive(res);

  // modify the QP to RTR
  modify_qp_to_rtr(res->qp, g_remote_con_data.qp_num, g_remote_con_data.lid,
                   g_remote_con_data.gid);

  // modify QP state to RTS
  modify_qp_to_rts(res->qp);

  return 0;
}

// This function will create and post a send work request.
inline int post_send(resources *res, ibv_wr_opcode opcode,
                     std::string_view msg) {
  struct ibv_send_wr sr;
  struct ibv_sge sge;
  struct ibv_send_wr *bad_wr = NULL;

  // prepare the scatter / gather entry
  memset(&sge, 0, sizeof(sge));

  strcpy(res->buf, msg.data());

  sge.addr = (uintptr_t)res->buf;
  sge.length = msg.size();
  sge.lkey = res->mr->lkey;

  // prepare the send work request
  memset(&sr, 0, sizeof(sr));

  sr.next = NULL;
  sr.wr_id = 0;
  sr.sg_list = &sge;

  sr.num_sge = 1;
  sr.opcode = opcode;
  sr.send_flags = IBV_SEND_SIGNALED;

  if (opcode != IBV_WR_SEND) {
    sr.wr.rdma.remote_addr = res->remote_props.addr;
    sr.wr.rdma.rkey = res->remote_props.rkey;
  }

  // there is a receive request in the responder side, so we won't get any
  // into RNR flow
  CHECK(ibv_post_send(res->qp, &sr, &bad_wr));

  switch (opcode) {
    case IBV_WR_SEND:
      ELOGV(INFO, "Send request was posted");
      break;
    case IBV_WR_RDMA_READ:
      ELOGV(INFO, "RDMA read request was posted");
      break;
    case IBV_WR_RDMA_WRITE:
      ELOGV(INFO, "RDMA write request was posted");
      break;
    default:
      ELOGV(INFO, "Unknown request was posted");
      break;
  }

  return 0;
}

inline int poll_completion(struct resources *res) {
  void *ev_ctx;
  int r = ibv_get_cq_event(res->complete_event_channel, &res->cq, &ev_ctx);
  assert(r >= 0);

  struct ibv_wc wc;
  int ne = ibv_poll_cq(res->cq, 1, &wc);
  if (ne < 0) {
    ELOGV(ERROR, "poll CQ failed %d", ne);
    exit(EXIT_FAILURE);
  }
  if (ne > 0) {
    ELOGV(INFO, "Completion was found in CQ with status %d\n", wc.status);
    assert(wc.status == IBV_WC_SUCCESS);
  }
  ibv_ack_cq_events(res->cq, ne);
  r = ibv_req_notify_cq(res->cq, 0);
  assert(r >= 0);

  // struct ibv_wc wc;
  // do {
  //   int ne = ibv_poll_cq(res->cq, 1, &wc);
  //   if (ne < 0) {
  //     ELOGV(ERROR, "poll CQ failed %d", ne);
  //     exit(EXIT_FAILURE);
  //   }
  //   if (ne > 0) {
  //     ELOGV(INFO, "Completion was found in CQ with status %d\n", wc.status);
  //     break;
  //   }
  // } while (true);
  return 0;
}

// Cleanup and deallocate all resources used
inline int resources_destroy(struct resources *res) {
  ibv_destroy_comp_channel(res->complete_event_channel);
  ibv_destroy_qp(res->qp);
  ibv_dereg_mr(res->mr);
  free(res->buf);
  ibv_destroy_cq(res->cq);
  ibv_dealloc_pd(res->pd);
  ibv_close_device(res->ib_ctx);

  return 0;
}

struct rdma_service_t {
  resources *res;
  union ibv_gid my_gid;
  std::atomic<int> count = 0;

  cm_con_data_t get_con_data(cm_con_data_t peer) {
    cm_con_data_t server_data{};
    server_data.addr = (uintptr_t)res->buf;
    server_data.rkey = res->mr->rkey;
    server_data.qp_num = res->qp->qp_num;
    server_data.lid = res->port_attr.lid;
    memcpy(server_data.gid, &my_gid, 16);

    g_remote_con_data = peer;
    memcpy(g_remote_con_data.gid, peer.gid, 16);

    connect_qp(res);

    return server_data;
  }

  void fetch() {
    std::string msg = "hello rdma! ";
    msg.append(std::to_string(++count));
    post_send(res, IBV_WR_SEND, msg);
    poll_completion(res);
  }

  void put() {
    poll_completion(res);
    std::cout << std::string_view(res->buf) << "\n";
    post_receive(res);
  }
};
/*-------------- rdma ---------------*/

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
