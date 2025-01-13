#ifndef _CORO_RDMA_TEST_COMMON_HPP_
#define _CORO_RDMA_TEST_COMMON_HPP_

#include <async_simple/coro/Lazy.h>
#include <rdmapp/cq.h>
#include <rdmapp/device.h>
#include <rdmapp/pd.h>
#include <rdmapp/qp.h>
#include <sys/wait.h>

#include <asio/buffer.hpp>
#include <asio/dispatch.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/posix/stream_descriptor.hpp>
#include <asio/streambuf.hpp>
#include <memory>
#include <ylt/coro_io/coro_io.hpp>
#include <ylt/coro_io/io_context_pool.hpp>
#include <ylt/coro_rpc/coro_rpc_client.hpp>
#include <ylt/coro_rpc/coro_rpc_server.hpp>

static inline async_simple::coro::Lazy<void> process_rdma_cq(
    asio::posix::stream_descriptor &cq_fd, rdmapp::comp_channel_ptr channel) {
  while (cq_fd.is_open()) {
    coro_io::callback_awaitor<std::error_code> awaitor;
    auto ec = co_await awaitor.await_resume([&cq_fd](auto handler) {
      cq_fd.async_wait(asio::posix::stream_descriptor::wait_read,
                       [handler](const auto &ec) mutable {
                         handler.set_value_then_resume(ec);
                       });
    });

    if (ec) {
      ELOG_INFO << "Failed to wait cq: " << ec;
      co_return;
    }

    auto cq = channel->get_event();
    cq->ack_event();
    cq->request_notify();

    struct ibv_wc wc;
    while (cq->poll(wc)) {
      rdmapp::process_wc(wc);
    }
  }
}

static inline async_simple::coro::Lazy<void> client_qp_demo_coro(
    rdmapp::qp_ptr qp) {
  char buffer[6];
  auto buffer_mr = std::make_unique<rdmapp::local_mr>(
      qp->pd()->reg_mr(&buffer[0], sizeof(buffer)));

  /* Send/Recv */
  auto [n, _] = co_await qp->recv(buffer_mr.get());
  ELOG_INFO << "Received " << n << " bytes from server: " << buffer;
  std::copy_n("world", sizeof(buffer), buffer);
  co_await qp->send(buffer_mr.get());
  ELOG_INFO << "Sent to server: " << buffer;

  /* Read/Write */
  char remote_mr_serialized[rdmapp::remote_mr::kSerializedSize];
  auto remote_mr_header_buffer = std::make_unique<rdmapp::local_mr>(
      qp->pd()->reg_mr(&remote_mr_serialized[0], sizeof(remote_mr_serialized)));
  co_await qp->recv(remote_mr_header_buffer.get());
  auto remote_mr = rdmapp::remote_mr::deserialize(remote_mr_serialized);
  ELOG_INFO << "Received mr addr=" << remote_mr.addr()
            << " length=" << remote_mr.length() << " rkey=" << remote_mr.rkey()
            << " from server";
  auto wc = co_await qp->read(remote_mr, buffer_mr.get());
  ELOG_INFO << "Read " << wc.byte_len << " bytes from server: " << buffer;
  std::copy_n("world", sizeof(buffer), buffer);
  co_await qp->write_with_imm(remote_mr, buffer_mr.get(), 1);

  /* Atomic Fetch-and-Add (FA)/Compare-and-Swap (CS) */
  char counter_mr_serialized[rdmapp::remote_mr::kSerializedSize];
  auto counter_mr_header_mr =
      std::make_unique<rdmapp::local_mr>(qp->pd()->reg_mr(
          &counter_mr_serialized[0], sizeof(counter_mr_serialized)));
  co_await qp->recv(counter_mr_header_mr.get());
  auto remote_counter_mr =
      rdmapp::remote_mr::deserialize(counter_mr_serialized);
  ELOG_INFO << "Received mr addr=" << remote_counter_mr.addr()
            << " length=" << remote_counter_mr.length()
            << " rkey=" << remote_counter_mr.rkey() << " from server";
  uint64_t counter = 0;
  auto local_counter_mr = std::make_unique<rdmapp::local_mr>(
      qp->pd()->reg_mr(&counter, sizeof(counter)));
  co_await qp->fetch_and_add(remote_counter_mr, local_counter_mr.get(), 1);
  ELOG_INFO << "Fetched and added from server: " << counter;
  co_await qp->write_with_imm(remote_mr, buffer_mr.get(), 1);
  co_await qp->compare_and_swap(remote_counter_mr, local_counter_mr.get(), 43,
                                4422);
  ELOG_INFO << "Compared and swapped from server: " << counter;
  co_await qp->write_with_imm(remote_mr, buffer_mr.get(), 1);

  co_return;
}

static inline async_simple::coro::Lazy<void> server_qp_demo_coro(
    rdmapp::qp_ptr qp) {
  /* Send/Recv */
  char buffer[6] = "hello";
  auto local_mr = std::make_unique<rdmapp::local_mr>(
      qp->pd()->reg_mr(&buffer[0], sizeof(buffer)));
  co_await qp->send(local_mr.get());
  ELOG_INFO << "Sent to client: " << buffer;
  co_await qp->recv(local_mr.get());
  ELOG_INFO << "Received from client: " << buffer;

  /* Read/Write */
  std::copy_n("hello", sizeof(buffer), buffer);
  auto local_mr_serialized = local_mr->serialize();
  auto local_mr_header_mr = std::make_unique<rdmapp::local_mr>(
      qp->pd()->reg_mr(&local_mr_serialized[0], local_mr_serialized.size()));
  co_await qp->send(local_mr_header_mr.get());
  ELOG_INFO << "Sent mr addr=" << local_mr->addr()
            << " length=" << local_mr->length() << " rkey=" << local_mr->rkey()
            << " to client";
  auto [_, imm] = co_await qp->recv(local_mr.get());
  assert(imm.has_value());
  ELOG_INFO << "Written by client (imm=" << imm.value() << "): " << buffer;

  /* Atomic */
  uint64_t counter = 42;
  auto counter_mr = std::make_unique<rdmapp::local_mr>(
      qp->pd()->reg_mr(&counter, sizeof(counter)));
  auto counter_mr_serialized = counter_mr->serialize();
  auto counter_mr_header_mr =
      std::make_unique<rdmapp::local_mr>(qp->pd()->reg_mr(
          &counter_mr_serialized[0], counter_mr_serialized.size()));
  co_await qp->send(&*counter_mr_header_mr);
  ELOG_INFO << "Sent mr addr=" << counter_mr->addr()
            << " length=" << counter_mr->length()
            << " rkey=" << counter_mr->rkey() << " to client";
  imm = (co_await qp->recv(local_mr.get())).second;
  assert(imm.has_value());
  ELOG_INFO << "Fetched and added by client: " << counter;
  imm = (co_await qp->recv(local_mr.get())).second;
  assert(imm.has_value());
  ELOG_INFO << "Compared and swapped by client: " << counter;
}
static inline async_simple::coro::Lazy<void> client_handle_qp(
    std::unique_ptr<rdmapp::qp> qp) {
  char buffer[6] = "hello";
  auto buffer_mr = std::make_unique<rdmapp::local_mr>(
      qp->pd()->reg_mr(&buffer[0], sizeof(buffer)));

  /* Send/Recv */
  co_await qp->send(buffer_mr.get());
  ELOG_INFO << "Sent to server: " << buffer;
  auto [n, _] = co_await qp->recv(buffer_mr.get());
  ELOG_INFO << "Received " << n << " bytes from server: " << buffer;

  /* Read/Write */
  char remote_mr_serialized[rdmapp::remote_mr::kSerializedSize];
  auto remote_mr_header_buffer = std::make_unique<rdmapp::local_mr>(
      qp->pd()->reg_mr(&remote_mr_serialized[0], sizeof(remote_mr_serialized)));
  co_await qp->recv(remote_mr_header_buffer.get());
  auto remote_mr = rdmapp::remote_mr::deserialize(remote_mr_serialized);
  ELOG_INFO << "Received mr addr=" << remote_mr.addr()
            << " length=" << remote_mr.length() << " rkey=" << remote_mr.rkey()
            << " from server";
  auto wc = co_await qp->read(remote_mr, buffer_mr.get());
  ELOG_INFO << "Read " << wc.byte_len << " bytes from server: " << buffer;
  std::copy_n("world", sizeof(buffer), buffer);
  co_await qp->write_with_imm(remote_mr, buffer_mr.get(), 1);

  /* Atomic Fetch-and-Add (FA)/Compare-and-Swap (CS) */
  char counter_mr_serialized[rdmapp::remote_mr::kSerializedSize];
  auto counter_mr_header_mr =
      std::make_unique<rdmapp::local_mr>(qp->pd()->reg_mr(
          &counter_mr_serialized[0], sizeof(counter_mr_serialized)));
  co_await qp->recv(counter_mr_header_mr.get());
  auto remote_counter_mr =
      rdmapp::remote_mr::deserialize(counter_mr_serialized);
  ELOG_INFO << "Received mr addr=" << remote_counter_mr.addr()
            << " length=" << remote_counter_mr.length()
            << " rkey=" << remote_counter_mr.rkey() << " from server";
  uint64_t counter = 0;
  auto local_counter_mr = std::make_unique<rdmapp::local_mr>(
      qp->pd()->reg_mr(&counter, sizeof(counter)));
  co_await qp->fetch_and_add(remote_counter_mr, local_counter_mr.get(), 1);
  ELOG_INFO << "Fetched and added from server: " << counter;
  co_await qp->write_with_imm(remote_mr, buffer_mr.get(), 1);

  // Synchroize
  co_await qp->recv(buffer_mr.get());

  co_await qp->compare_and_swap(remote_counter_mr, local_counter_mr.get(), 43,
                                4422);
  ELOG_INFO << "Compared and swapped from server: " << counter;
  co_await qp->write_with_imm(remote_mr, buffer_mr.get(), 1);

  co_return;
}

static inline async_simple::coro::Lazy<void> server_handle_qp(
    std::unique_ptr<rdmapp::qp> qp,
    std::atomic<size_t> *served_qp_count = nullptr) {
  /* Send/Recv */
  char buffer[6];
  auto local_mr = std::make_unique<rdmapp::local_mr>(
      qp->pd()->reg_mr(&buffer[0], sizeof(buffer)));
  co_await qp->recv(local_mr.get());
  ELOG_INFO << "Received from client: " << buffer;
  std::copy_n("world", sizeof(buffer), buffer);
  co_await qp->send(local_mr.get());
  ELOG_INFO << "Sent to client: " << buffer;

  /* Read/Write */
  std::copy_n("hello", sizeof(buffer), buffer);
  auto local_mr_serialized = local_mr->serialize();
  auto local_mr_header_mr = std::make_unique<rdmapp::local_mr>(
      qp->pd()->reg_mr(&local_mr_serialized[0], local_mr_serialized.size()));
  co_await qp->send(local_mr_header_mr.get());
  ELOG_INFO << "Sent mr addr=" << local_mr->addr()
            << " length=" << local_mr->length() << " rkey=" << local_mr->rkey()
            << " to client";
  auto [_, imm] = co_await qp->recv(local_mr.get());
  assert(imm.has_value());
  ELOG_INFO << "Written by client (imm=" << imm.value() << "): " << buffer;

  /* Atomic */
  uint64_t counter = 42;
  auto counter_mr = std::make_unique<rdmapp::local_mr>(
      qp->pd()->reg_mr(&counter, sizeof(counter)));
  auto counter_mr_serialized = counter_mr->serialize();
  auto counter_mr_header_mr =
      std::make_unique<rdmapp::local_mr>(qp->pd()->reg_mr(
          &counter_mr_serialized[0], counter_mr_serialized.size()));
  co_await qp->send(&*counter_mr_header_mr);
  ELOG_INFO << "Sent mr addr=" << counter_mr->addr()
            << " length=" << counter_mr->length()
            << " rkey=" << counter_mr->rkey() << " to client";
  imm = (co_await qp->recv(local_mr.get())).second;
  assert(imm.has_value());
  ELOG_INFO << "Fetched and added by client: " << counter;

  // Synchronize
  co_await qp->send(local_mr.get());

  imm = (co_await qp->recv(local_mr.get())).second;
  assert(imm.has_value());
  ELOG_INFO << "Compared and swapped by client: " << counter;

  if (served_qp_count) {
    served_qp_count->fetch_add(1);
  }
}

struct rdma_environment {
  std::unique_ptr<rdmapp::device> device_;
  std::unique_ptr<rdmapp::pd> pd_;
  std::unique_ptr<rdmapp::comp_channel> channel_;
  std::unique_ptr<rdmapp::cq> cq_;

  rdma_environment() {
    device_ = std::make_unique<rdmapp::device>();

    pd_ = std::make_unique<rdmapp::pd>(&*device_);

    channel_ = std::make_unique<rdmapp::comp_channel>(&*device_);
    channel_->set_non_blocking();

    cq_ = std::make_unique<rdmapp::cq>(&*device_, 128, &*channel_);
    cq_->request_notify();
  }
};

class coro_rdma_demo_service {
  coro_io::ExecutorWrapper<> *rdma_executor_;
  rdma_environment rdma_env_;
  asio::posix::stream_descriptor cq_fd_;

  void start_rdma_cq_worker() {
    process_rdma_cq(cq_fd_, &*rdma_env_.channel_).via(rdma_executor_).detach();
  }

 public:
  std::atomic<size_t> served_qp_count_ = 0;

  coro_rdma_demo_service(coro_io::ExecutorWrapper<> *executor)
      : rdma_executor_(executor),
        cq_fd_(executor->context(), rdma_env_.channel_->fd()) {
    start_rdma_cq_worker();
  }

  void stop_rdma_cq_worker() {
    cq_fd_.cancel();
    cq_fd_.close();
  }

  async_simple::coro::Lazy<rdmapp::qp::header_info> qp_handshake(
      rdmapp::qp::header_info remote_qp) {
    union ibv_gid gid;
    ::memcpy(&gid, &remote_qp.raw_gid[0], sizeof(union ibv_gid));
    auto qp = std::make_unique<rdmapp::qp>(remote_qp.lid, remote_qp.qpn,
                                           remote_qp.psn, gid, &*rdma_env_.pd_,
                                           &*rdma_env_.cq_, &*rdma_env_.cq_);
    auto header = qp->header();
    server_handle_qp(std::move(qp), &served_qp_count_)
        .via(rdma_executor_)
        .detach();
    co_return header;
  }
};

class coro_rdma_demo_client {
  coro_rpc::coro_rpc_client rpc_client_;
  coro_io::ExecutorWrapper<> *rdma_executor_;
  rdma_environment rdma_env_;
  asio::posix::stream_descriptor cq_fd_;

  void start_rdma_cq_worker() {
    process_rdma_cq(cq_fd_, &*rdma_env_.channel_).via(rdma_executor_).detach();
  }

 public:
  coro_rdma_demo_client()
      : rdma_executor_(&rpc_client_.get_executor()),
        cq_fd_(rdma_executor_->context(), rdma_env_.channel_->fd()) {
    start_rdma_cq_worker();
  }

  void stop_rdma_cq_worker() {
    cq_fd_.cancel();
    cq_fd_.close();
  }

  async_simple::coro::Lazy<void> run(std::string const &address,
                                     std::string const &port) {
    auto ec = co_await rpc_client_.connect(address, port);
    if (ec) {
      ELOG_ERROR << "connect failed: " << ec.message();
      co_return;
    }

    auto qp = std::make_unique<rdmapp::qp>(&*rdma_env_.pd_, &*rdma_env_.cq_,
                                           &*rdma_env_.cq_);
    auto ret = co_await rpc_client_.call<&coro_rdma_demo_service::qp_handshake>(
        qp->header());
    auto remote_qp = ret.value();
    union ibv_gid gid;
    ::memcpy(&gid, &remote_qp.raw_gid[0], sizeof(union ibv_gid));
    qp->rtr(remote_qp.lid, remote_qp.qpn, remote_qp.psn, gid);
    qp->rts();

    co_await client_handle_qp(std::move(qp));
    stop_rdma_cq_worker();
  }
};

#endif