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
#include <asio/streambuf.hpp>
#include <memory>
#include <ylt/coro_io/coro_io.hpp>
#include <ylt/coro_io/io_context_pool.hpp>

#include "asio/posix/stream_descriptor.hpp"

static inline async_simple::coro::Lazy<void> process_rdma_cq(
    asio::io_context &ctx, rdmapp::comp_channel_ptr channel) {
  asio::posix::stream_descriptor cq_fd(ctx.get_executor(), channel->fd());
  while (!ctx.stopped()) {
    coro_io::callback_awaitor<std::error_code> awaitor;
    auto ec = co_await awaitor.await_resume([&cq_fd](auto handler) {
      cq_fd.async_wait(asio::posix::stream_descriptor::wait_read,
                       [handler](const auto &ec) mutable {
                         handler.set_value_then_resume(ec);
                       });
    });

    if (ec) {
      std::cerr << "failed to wait: " << ec.message() << std::endl;
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

static inline async_simple::coro::Lazy<void> client_qp_demo_coro(rdmapp::qp_ptr qp) {
  char buffer[6];
  auto buffer_mr = std::make_unique<rdmapp::local_mr>(
      qp->pd()->reg_mr(&buffer[0], sizeof(buffer)));

  /* Send/Recv */
  auto [n, _] = co_await qp->recv(buffer_mr.get());
  std::cout << "Received " << n << " bytes from server: " << buffer
            << std::endl;
  std::copy_n("world", sizeof(buffer), buffer);
  co_await qp->send(buffer_mr.get());
  std::cout << "Sent to server: " << buffer << std::endl;

  /* Read/Write */
  char remote_mr_serialized[rdmapp::remote_mr::kSerializedSize];
  auto remote_mr_header_buffer = std::make_unique<rdmapp::local_mr>(
      qp->pd()->reg_mr(&remote_mr_serialized[0], sizeof(remote_mr_serialized)));
  co_await qp->recv(remote_mr_header_buffer.get());
  auto remote_mr = rdmapp::remote_mr::deserialize(remote_mr_serialized);
  std::cout << "Received mr addr=" << remote_mr.addr()
            << " length=" << remote_mr.length() << " rkey=" << remote_mr.rkey()
            << " from server" << std::endl;
  auto wc = co_await qp->read(remote_mr, buffer_mr.get());
  std::cout << "Read " << wc.byte_len << " bytes from server: " << buffer
            << std::endl;
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
  std::cout << "Received mr addr=" << remote_counter_mr.addr()
            << " length=" << remote_counter_mr.length()
            << " rkey=" << remote_counter_mr.rkey() << " from server"
            << std::endl;
  uint64_t counter = 0;
  auto local_counter_mr = std::make_unique<rdmapp::local_mr>(
      qp->pd()->reg_mr(&counter, sizeof(counter)));
  co_await qp->fetch_and_add(remote_counter_mr, local_counter_mr.get(), 1);
  std::cout << "Fetched and added from server: " << counter << std::endl;
  co_await qp->write_with_imm(remote_mr, buffer_mr.get(), 1);
  co_await qp->compare_and_swap(remote_counter_mr, local_counter_mr.get(), 43,
                                4422);
  std::cout << "Compared and swapped from server: " << counter << std::endl;
  co_await qp->write_with_imm(remote_mr, buffer_mr.get(), 1);

  co_return;
}

static inline async_simple::coro::Lazy<void> server_qp_demo_coro(rdmapp::qp_ptr qp) {
  /* Send/Recv */
  char buffer[6] = "hello";
  auto local_mr = std::make_unique<rdmapp::local_mr>(
      qp->pd()->reg_mr(&buffer[0], sizeof(buffer)));
  co_await qp->send(local_mr.get());
  std::cout << "Sent to client: " << buffer << std::endl;
  co_await qp->recv(local_mr.get());
  std::cout << "Received from client: " << buffer << std::endl;

  /* Read/Write */
  std::copy_n("hello", sizeof(buffer), buffer);
  auto local_mr_serialized = local_mr->serialize();
  auto local_mr_header_mr = std::make_unique<rdmapp::local_mr>(
      qp->pd()->reg_mr(&local_mr_serialized[0], local_mr_serialized.size()));
  co_await qp->send(local_mr_header_mr.get());
  std::cout << "Sent mr addr=" << local_mr->addr()
            << " length=" << local_mr->length() << " rkey=" << local_mr->rkey()
            << " to client" << std::endl;
  auto [_, imm] = co_await qp->recv(local_mr.get());
  assert(imm.has_value());
  std::cout << "Written by client (imm=" << imm.value() << "): " << buffer
            << std::endl;

  /* Atomic */
  uint64_t counter = 42;
  auto counter_mr = std::make_unique<rdmapp::local_mr>(
      qp->pd()->reg_mr(&counter, sizeof(counter)));
  auto counter_mr_serialized = counter_mr->serialize();
  auto counter_mr_header_mr =
      std::make_unique<rdmapp::local_mr>(qp->pd()->reg_mr(
          &counter_mr_serialized[0], counter_mr_serialized.size()));
  co_await qp->send(&*counter_mr_header_mr);
  std::cout << "Sent mr addr=" << counter_mr->addr()
            << " length=" << counter_mr->length()
            << " rkey=" << counter_mr->rkey() << " to client" << std::endl;
  imm = (co_await qp->recv(local_mr.get())).second;
  assert(imm.has_value());
  std::cout << "Fetched and added by client: " << counter << std::endl;
  imm = (co_await qp->recv(local_mr.get())).second;
  assert(imm.has_value());
  std::cout << "Compared and swapped by client: " << counter << std::endl;
}

static inline async_simple::coro::Lazy<rdmapp::deserialized_qp> recv_qp(
    asio::ip::tcp::socket &socket) {
  std::array<uint8_t, rdmapp::deserialized_qp::qp_header::kSerializedSize>
      header;
  {
    auto [ec, size] =
        co_await coro_io::async_read(socket, asio::buffer(header));
    if (ec) {
      std::cerr << ec.message() << std::endl;
      throw std::runtime_error("read qp header failed");
    }
  }
  auto remote_qp = rdmapp::deserialized_qp::deserialize(header.data());
  auto const remote_gid_str =
      rdmapp::device::gid_hex_string(remote_qp.header.gid);
  fprintf(stderr,
          "received header gid=%s lid=%u qpn=%u psn=%u user_data_size=%u\n",
          remote_gid_str.c_str(), remote_qp.header.lid, remote_qp.header.qp_num,
          remote_qp.header.sq_psn, remote_qp.header.user_data_size);
  if (remote_qp.header.user_data_size > 0) {
    remote_qp.user_data.resize(remote_qp.header.user_data_size);
    auto [ec, size] =
        co_await coro_io::async_read(socket, asio::buffer(remote_qp.user_data));
    if (ec) {
      std::cerr << ec.message() << std::endl;
      throw std::runtime_error("read qp user data failed");
    }
  }
  co_return remote_qp;
}

static inline async_simple::coro::Lazy<std::error_code> send_qp(asio::ip::tcp::socket &socket,
                                                  rdmapp::qp const &qp) {
  auto serialized_qp = qp.serialize();
  auto [ec, size] =
      co_await coro_io::async_write(socket, asio::buffer(serialized_qp));
  co_return ec;
}

class rdma_qp_client {
  asio::io_context *ctx_;
  coro_io::ExecutorWrapper<> executor_;
  uint16_t port_;
  std::string address_;
  std::error_code errc_ = {};
  asio::ip::tcp::socket socket_;
  rdmapp::pd_ptr pd_;
  rdmapp::cq_ptr cq_;

 public:
  rdma_qp_client(asio::io_context &ctx, rdmapp::pd_ptr pd, rdmapp::cq_ptr cq,
                 unsigned short port, std::string address = "0.0.0.0")
      : ctx_(&ctx),
        pd_(pd),
        cq_(cq),
        executor_(ctx.get_executor()),
        port_(port),
        address_(address),
        socket_(ctx) {}

  async_simple::coro::Lazy<void> handle_qp(rdmapp::qp_ptr qp) {
    char buffer[6];
    auto buffer_mr = std::make_unique<rdmapp::local_mr>(
        qp->pd()->reg_mr(&buffer[0], sizeof(buffer)));

    /* Send/Recv */
    auto [n, _] = co_await qp->recv(buffer_mr.get());
    std::cout << "Received " << n << " bytes from server: " << buffer
              << std::endl;
    std::copy_n("world", sizeof(buffer), buffer);
    co_await qp->send(buffer_mr.get());
    std::cout << "Sent to server: " << buffer << std::endl;

    /* Read/Write */
    char remote_mr_serialized[rdmapp::remote_mr::kSerializedSize];
    auto remote_mr_header_buffer =
        std::make_unique<rdmapp::local_mr>(qp->pd()->reg_mr(
            &remote_mr_serialized[0], sizeof(remote_mr_serialized)));
    co_await qp->recv(remote_mr_header_buffer.get());
    auto remote_mr = rdmapp::remote_mr::deserialize(remote_mr_serialized);
    std::cout << "Received mr addr=" << remote_mr.addr()
              << " length=" << remote_mr.length()
              << " rkey=" << remote_mr.rkey() << " from server" << std::endl;
    auto wc = co_await qp->read(remote_mr, buffer_mr.get());
    std::cout << "Read " << wc.byte_len << " bytes from server: " << buffer
              << std::endl;
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
    std::cout << "Received mr addr=" << remote_counter_mr.addr()
              << " length=" << remote_counter_mr.length()
              << " rkey=" << remote_counter_mr.rkey() << " from server"
              << std::endl;
    uint64_t counter = 0;
    auto local_counter_mr = std::make_unique<rdmapp::local_mr>(
        qp->pd()->reg_mr(&counter, sizeof(counter)));
    co_await qp->fetch_and_add(remote_counter_mr, local_counter_mr.get(), 1);
    std::cout << "Fetched and added from server: " << counter << std::endl;
    co_await qp->write_with_imm(remote_mr, buffer_mr.get(), 1);
    co_await qp->compare_and_swap(remote_counter_mr, local_counter_mr.get(), 43,
                                  4422);
    std::cout << "Compared and swapped from server: " << counter << std::endl;
    co_await qp->write_with_imm(remote_mr, buffer_mr.get(), 1);

    co_return;
  }

  async_simple::coro::Lazy<void> handle_server_connection(
      asio::ip::tcp::socket &socket) {
    auto qp = std::make_unique<rdmapp::qp>(pd_, cq_, cq_);
    co_await send_qp(socket, *qp);
    std::cerr << "send qp success" << std::endl;
    auto remote_qp = co_await recv_qp(socket);
    std::cerr << "recv qp success" << std::endl;
    qp->rtr(remote_qp.header.lid, remote_qp.header.qp_num,
            remote_qp.header.sq_psn, remote_qp.header.gid);
    qp->user_data() = std::move(remote_qp.user_data);
    qp->rts();
    co_await handle_qp(qp.get());
  }

  async_simple::coro::Lazy<void> run() {
    auto ec = co_await coro_io::async_connect(&executor_, socket_, address_,
                                              std::to_string(port_));
    if (ec) {
      throw std::system_error(ec, "connect failed");
    }
    co_await handle_server_connection(socket_);
  }
};

class rdma_qp_server {
  asio::io_context *ctx_;
  coro_io::ExecutorWrapper<> executor_;
  uint16_t port_;
  std::string address_;
  std::error_code errc_ = {};
  asio::ip::tcp::acceptor acceptor_;
  std::promise<void> acceptor_close_waiter_;
  rdmapp::pd_ptr pd_;
  rdmapp::cq_ptr cq_;

 public:
  rdma_qp_server(asio::io_context &ctx, rdmapp::pd_ptr pd, rdmapp::cq_ptr cq,
                 unsigned short port, std::string address = "0.0.0.0")
      : ctx_(&ctx),
        pd_(pd),
        cq_(cq),
        executor_(ctx.get_executor()),
        port_(port),
        address_(address),
        acceptor_(ctx) {}

  std::error_code listen() {
    asio::error_code ec;

    asio::ip::tcp::resolver::query query(address_, std::to_string(port_));
    asio::ip::tcp::resolver resolver(acceptor_.get_executor());
    asio::ip::tcp::resolver::iterator it = resolver.resolve(query, ec);

    asio::ip::tcp::resolver::iterator it_end;
    if (ec || it == it_end) {
      if (ec) {
        return ec;
      }
      return std::make_error_code(std::errc::address_not_available);
    }

    auto endpoint = it->endpoint();
    ec = acceptor_.open(endpoint.protocol(), ec);

    if (ec) {
      return ec;
    }
#ifdef __GNUC__
    ec = acceptor_.set_option(asio::ip::tcp::acceptor::reuse_address(true), ec);
#endif
    ec = acceptor_.bind(endpoint, ec);
    if (ec) {
      std::error_code ignore_ec;
      ignore_ec = acceptor_.cancel(ignore_ec);
      ignore_ec = acceptor_.close(ignore_ec);
      return ec;
    }
    ec = acceptor_.listen(asio::socket_base::max_listen_connections, ec);
    if (ec) {
      return ec;
    }
    auto local_ep = acceptor_.local_endpoint(ec);
    if (ec) {
      return ec;
    }
    port_ = local_ep.port();
    std::cerr << "listen success, port: " << port_ << std::endl;
    return {};
  }

  async_simple::coro::Lazy<void> handle_qp(rdmapp::qp_ptr qp) {
    /* Send/Recv */
    char buffer[6] = "hello";
    auto local_mr = std::make_unique<rdmapp::local_mr>(
        qp->pd()->reg_mr(&buffer[0], sizeof(buffer)));
    co_await qp->send(local_mr.get());
    std::cout << "Sent to client: " << buffer << std::endl;
    co_await qp->recv(local_mr.get());
    std::cout << "Received from client: " << buffer << std::endl;

    /* Read/Write */
    std::copy_n("hello", sizeof(buffer), buffer);
    auto local_mr_serialized = local_mr->serialize();
    auto local_mr_header_mr = std::make_unique<rdmapp::local_mr>(
        qp->pd()->reg_mr(&local_mr_serialized[0], local_mr_serialized.size()));
    co_await qp->send(local_mr_header_mr.get());
    std::cout << "Sent mr addr=" << local_mr->addr()
              << " length=" << local_mr->length()
              << " rkey=" << local_mr->rkey() << " to client" << std::endl;
    auto [_, imm] = co_await qp->recv(local_mr.get());
    assert(imm.has_value());
    std::cout << "Written by client (imm=" << imm.value() << "): " << buffer
              << std::endl;

    /* Atomic */
    uint64_t counter = 42;
    auto counter_mr = std::make_unique<rdmapp::local_mr>(
        qp->pd()->reg_mr(&counter, sizeof(counter)));
    auto counter_mr_serialized = counter_mr->serialize();
    auto counter_mr_header_mr =
        std::make_unique<rdmapp::local_mr>(qp->pd()->reg_mr(
            &counter_mr_serialized[0], counter_mr_serialized.size()));
    co_await qp->send(&*counter_mr_header_mr);
    std::cout << "Sent mr addr=" << counter_mr->addr()
              << " length=" << counter_mr->length()
              << " rkey=" << counter_mr->rkey() << " to client" << std::endl;
    imm = (co_await qp->recv(local_mr.get())).second;
    assert(imm.has_value());
    std::cout << "Fetched and added by client: " << counter << std::endl;
    imm = (co_await qp->recv(local_mr.get())).second;
    assert(imm.has_value());
    std::cout << "Compared and swapped by client: " << counter << std::endl;
  }

  async_simple::coro::Lazy<void> handle_client_connection(
      asio::ip::tcp::socket socket) {
    auto remote_qp = co_await recv_qp(socket);
    auto qp = std::make_unique<rdmapp::qp>(
        remote_qp.header.lid, remote_qp.header.qp_num, remote_qp.header.sq_psn,
        remote_qp.header.gid, pd_, cq_, cq_);
    std::cerr << "recv qp success" << std::endl;
    co_await send_qp(socket, *qp);
    std::cerr << "send qp success" << std::endl;
    co_await handle_qp(&*qp);
  }

  async_simple::coro::Lazy<> run() {
    asio::ip::tcp::socket socket(executor_.get_asio_executor());
    auto ec = co_await coro_io::async_accept(acceptor_, socket);
    if (ec) {
      std::cerr << "accept failed: " << ec.message() << std::endl;
      if (ec == asio::error::operation_aborted ||
          ec == asio::error::bad_descriptor) {
        acceptor_close_waiter_.set_value();
        co_return;
      }
    }
    std::cout << "accpeted connection " << socket.remote_endpoint()
              << std::endl;
    co_await handle_client_connection(std::move(socket));
  }

  async_simple::coro::Lazy<void> loop() {
    for (;;) {
      co_await run();
    }
  }
};

#endif