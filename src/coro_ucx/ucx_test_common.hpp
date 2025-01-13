#ifndef _CORO_UCX_TEST_COMMON_HPP_
#define _CORO_UCX_TEST_COMMON_HPP_

#include <async_simple/coro/Lazy.h>
#include <async_simple/coro/Semaphore.h>
#include <sys/wait.h>
#include <ucxpp/endpoint.h>
#include <ucxpp/worker.h>

#include <asio/buffer.hpp>
#include <asio/dispatch.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/posix/stream_descriptor.hpp>
#include <asio/streambuf.hpp>
#include <ylt/coro_io/coro_io.hpp>
#include <ylt/coro_io/io_context_pool.hpp>
#include <ylt/coro_rpc/coro_rpc_client.hpp>
#include <ylt/coro_rpc/coro_rpc_server.hpp>

constexpr ucp_tag_t kTestTag = 0xFD709394UL;
constexpr ucp_tag_t kBellTag = 0xbe11be11UL;

static inline async_simple::coro::Lazy<void> ucx_event_loop(
    asio::io_context &ctx, ucxpp::context &context,
    std::function<void(ucxpp::worker_ptr, asio::posix::stream_descriptor &)>
        worker_cb) {
  auto worker = new ucxpp::worker(&context);
  asio::posix::stream_descriptor worker_fd(ctx.get_executor(),
                                           worker->event_fd());
  worker_cb(worker, worker_fd);
  while (worker_fd.is_open()) {
    coro_io::callback_awaitor<std::error_code> awaitor;
    auto ec = co_await awaitor.await_resume([&worker_fd](auto handler) {
      worker_fd.async_wait(asio::posix::stream_descriptor::wait_read,
                           [handler](const auto &ec) mutable {
                             handler.set_value_then_resume(ec);
                           });
    });

    if (ec) {
      ELOG_INFO << "failed to wait: " << ec.message();
      co_return;
    }

    do {
      while (worker->progress());
    } while (!worker->arm());
  }
}

static inline async_simple::coro::Lazy<ucxpp::endpoint_ptr> recv_ep(
    ucxpp::worker_ptr worker, asio::ip::tcp::socket &socket) {
  std::array<uint8_t, sizeof(size_t)> address_length_buf;
  {
    auto [ec, size] =
        co_await coro_io::async_read(socket, asio::buffer(address_length_buf));
    if (ec) {
      std::cerr << ec.message();
      throw std::runtime_error("read address length failed");
    }
  }

  size_t address_length = 0;
  auto p = address_length_buf.data();
  ucxpp::detail::deserialize(p, address_length);

  std::vector<char> address_buf(address_length);
  {
    auto [ec, size] =
        co_await coro_io::async_read(socket, asio::buffer(address_buf));
    if (ec) {
      std::cerr << ec.message();
      throw std::runtime_error("read address failed");
    }
  }

  auto remote_addr = ucxpp::remote_address(std::move(address_buf));
  co_return new ucxpp::endpoint(worker, remote_addr);
}

static inline async_simple::coro::Lazy<void> send_ep(
    ucxpp::worker::local_address const &address,
    asio::ip::tcp::socket &socket) {
  auto [ec, size] =
      co_await coro_io::async_write(socket, asio::buffer(address.serialize()));
}

static inline async_simple::coro::Lazy<
    std::pair<uint64_t, ucxpp::remote_memory_handle>>
receive_mr(ucxpp::endpoint_ptr ep) {
  uint64_t remote_addr;
  co_await ep->stream_recv(&remote_addr, sizeof(remote_addr));
  remote_addr = ::be64toh(remote_addr);
  size_t rkey_length;
  co_await ep->stream_recv(&rkey_length, sizeof(rkey_length));
  rkey_length = ::be64toh(rkey_length);
  std::vector<char> rkey_buffer(rkey_length);
  size_t rkey_recved = 0;
  while (rkey_recved < rkey_length) {
    auto n = co_await ep->stream_recv(&rkey_buffer[rkey_recved],
                                      rkey_length - rkey_recved);
    rkey_recved += n;
  }
  co_return std::make_pair(remote_addr,
                           ucxpp::remote_memory_handle(ep, rkey_buffer.data()));
}

static inline async_simple::coro::Lazy<void> ucx_client_handle_ep(
    std::unique_ptr<ucxpp::endpoint> ep) {
  ep->print();
  char buffer[6] = "hello";

  /* Tag Send/Recv */
  co_await ep->tag_send(buffer, sizeof(buffer), kTestTag);
  auto [n, sender_tag] =
      co_await ep->worker()->tag_recv(buffer, sizeof(buffer), kTestTag);
  ELOG_INFO << "Received " << n << " bytes from " << std::hex << sender_tag
            << std::dec << ": " << buffer;

  /* Stream Send/Recv */
  n = co_await ep->stream_recv(buffer, sizeof(buffer));
  ELOG_INFO << "Received " << n << " bytes: " << buffer;
  std::copy_n("world", 6, buffer);
  co_await ep->stream_send(buffer, sizeof(buffer));

  /* RMA Get/Put */
  auto local_mr = ucxpp::local_memory_handle::register_mem(
      ep->worker()->context(), buffer, sizeof(buffer));
  auto [remote_addr, remote_mr] = co_await receive_mr(&*ep);
  ELOG_INFO << "Remote addr: 0x" << std::hex << remote_addr << std::dec;
  remote_mr.get(buffer, sizeof(buffer), remote_addr);
  co_await ep->flush();
  ELOG_INFO << "Read from server: " << buffer;
  std::copy_n("world", 6, buffer);
  remote_mr.put(buffer, sizeof(buffer), remote_addr);
  co_await ep->flush();
  ELOG_INFO << "Wrote to server: " << buffer;
  size_t bell;
  co_await ep->tag_send(&bell, sizeof(bell), kBellTag);

  /* Atomic */
  uint64_t local_value = 1;
  uint64_t reply_value = 0;
  auto [atomic_raddr, atomic_mr] = co_await receive_mr(&*ep);

  /* Fetch and Add */
  co_await atomic_mr.atomic_fetch_add(atomic_raddr, local_value, reply_value);
  ELOG_INFO << "Fetched and added on server: " << reply_value;
  co_await ep->tag_send(&bell, sizeof(bell), kBellTag);
  co_await ep->worker()->tag_recv(&bell, sizeof(bell), kBellTag);

  /* Compare and Swap */
  local_value = reply_value + local_value;
  reply_value = 456;
  co_await atomic_mr.atomic_compare_swap(atomic_raddr, local_value,
                                         reply_value);
  ELOG_INFO << "Compared and swapped on server: " << reply_value;
  co_await ep->tag_send(&bell, sizeof(bell), kBellTag);
  co_await ep->worker()->tag_recv(&bell, sizeof(bell), kBellTag);

  /* Swap */
  local_value = 123;
  co_await atomic_mr.atomic_swap(atomic_raddr, local_value, reply_value);
  ELOG_INFO << "Swapped on server: " << reply_value;
  co_await ep->tag_send(&bell, sizeof(bell), kBellTag);
  co_await ep->worker()->tag_recv(&bell, sizeof(bell), kBellTag);

  /* Fetch and And */
  local_value = 0xF;
  co_await atomic_mr.atomic_fetch_and(atomic_raddr, local_value, reply_value);
  ELOG_INFO << "Fetched and anded on server: " << reply_value;
  co_await ep->tag_send(&bell, sizeof(bell), kBellTag);
  co_await ep->worker()->tag_recv(&bell, sizeof(bell), kBellTag);

  /* Fetch and Or */
  local_value = 0xF;
  co_await atomic_mr.atomic_fetch_or(atomic_raddr, local_value, reply_value);
  ELOG_INFO << "Fetched and ored on server: " << reply_value;
  co_await ep->tag_send(&bell, sizeof(bell), kBellTag);
  co_await ep->worker()->tag_recv(&bell, sizeof(bell), kBellTag);

  /* Fetch and Xor */
  local_value = 0xF;
  co_await atomic_mr.atomic_fetch_xor(atomic_raddr, local_value, reply_value);
  ELOG_INFO << "Fetched and xored on server: " << reply_value;
  co_await ep->tag_send(&bell, sizeof(bell), kBellTag);
  co_await ep->worker()->tag_recv(&bell, sizeof(bell), kBellTag);

  co_await ep->flush();
  co_await ep->close();

  ELOG_INFO << "Client finished";

  co_return;
}

static inline async_simple::coro::Lazy<void> send_mr(
    ucxpp::endpoint_ptr ep, void *address,
    ucxpp::local_memory_handle const &local_mr) {
  auto packed_rkey = local_mr.pack_rkey();
  auto rkey_length = ::htobe64(packed_rkey.get_length());
  auto remote_addr = ::htobe64(reinterpret_cast<uint64_t>(address));
  co_await ep->stream_send(&remote_addr, sizeof(remote_addr));
  co_await ep->stream_send(&rkey_length, sizeof(rkey_length));
  co_await ep->stream_send(packed_rkey.get_buffer(), packed_rkey.get_length());
  co_return;
}

static inline async_simple::coro::Lazy<void> ucx_server_handle_ep(
    std::unique_ptr<ucxpp::endpoint> ep,
    std::atomic<size_t> *served_ep_count = nullptr) {
  ep->print();
  char buffer[6] = "Hello";

  /* Tag Send/Recv */
  auto [n, sender_tag] =
      co_await ep->worker()->tag_recv(buffer, sizeof(buffer), kTestTag);
  ELOG_INFO << "Received " << n << " bytes from " << std::hex << sender_tag
            << std::dec << ": " << buffer;
  std::copy_n("world", 6, buffer);
  co_await ep->tag_send(buffer, sizeof(buffer), kTestTag);

  /* Stream Send/Recv */
  std::copy_n("Hello", 6, buffer);
  co_await ep->stream_send(buffer, sizeof(buffer));
  n = co_await ep->stream_recv(buffer, sizeof(buffer));
  ELOG_INFO << "Received " << n << " bytes: " << buffer;

  /* RMA Get/Put */
  std::copy_n("Hello", 6, buffer);
  auto local_mr = ucxpp::local_memory_handle::register_mem(
      ep->worker()->context(), buffer, sizeof(buffer));
  co_await send_mr(&*ep, buffer, local_mr);

  size_t bell;
  co_await ep->worker()->tag_recv(&bell, sizeof(bell), kBellTag);
  ELOG_INFO << "Written by client: " << buffer;

  /* Atomic */
  uint64_t value = 42;
  auto atomic_mr = ucxpp::local_memory_handle::register_mem(
      ep->worker()->context(), &value, sizeof(value));
  co_await send_mr(&*ep, &value, atomic_mr);

  co_await ep->worker()->tag_recv(&bell, sizeof(bell), kBellTag);
  ELOG_INFO << "Fetched and added by client: " << value;
  co_await ep->tag_send(&bell, sizeof(bell), kBellTag);

  co_await ep->worker()->tag_recv(&bell, sizeof(bell), kBellTag);
  ELOG_INFO << "Compared and Swapped by client: " << value;
  co_await ep->tag_send(&bell, sizeof(bell), kBellTag);

  co_await ep->worker()->tag_recv(&bell, sizeof(bell), kBellTag);
  ELOG_INFO << "Swapped by client: " << value;
  co_await ep->tag_send(&bell, sizeof(bell), kBellTag);

  co_await ep->worker()->tag_recv(&bell, sizeof(bell), kBellTag);
  ELOG_INFO << "Fetched and Anded by client: " << value;
  co_await ep->tag_send(&bell, sizeof(bell), kBellTag);

  co_await ep->worker()->tag_recv(&bell, sizeof(bell), kBellTag);
  ELOG_INFO << "Fetched and Ored by client: " << value;
  co_await ep->tag_send(&bell, sizeof(bell), kBellTag);

  co_await ep->worker()->tag_recv(&bell, sizeof(bell), kBellTag);
  ELOG_INFO << "Fetched and Xored by client: " << value;
  co_await ep->tag_send(&bell, sizeof(bell), kBellTag);

  co_await ep->flush();
  co_await ep->close();

  if (served_ep_count) {
    served_ep_count->fetch_add(1);
  }

  ELOG_INFO << "Server finished";

  co_return;
}

struct ucx_environment {
  std::unique_ptr<ucxpp::context> ctx_;
  ucx_environment() {
    ctx_.reset(ucxpp::context::builder()
                   .enable_stream()
                   .enable_tag()
                   .enable_wakeup()
                   .enable_rma()
                   .enable_amo64()
                   .build());
  }
};

class coro_ucx_demo_service {
  coro_io::ExecutorWrapper<> *ucx_executor_;
  ucx_environment ucx_env_;
  ucxpp::worker_ptr worker_;
  asio::posix::stream_descriptor *worker_fd_;

 public:
  std::atomic<size_t> served_ep_count_ = 0;

  coro_ucx_demo_service(coro_io::ExecutorWrapper<> *executor)
      : ucx_executor_(executor) {
    ucx_event_loop(ucx_executor_->context(), *ucx_env_.ctx_,
                   [this](ucxpp::worker_ptr worker,
                          asio::posix::stream_descriptor &worker_fd) {
                     worker_ = worker;
                     worker_fd_ = &worker_fd;
                   })
        .via(ucx_executor_)
        .detach();
  }

  void stop_ucx_worker() {
    delete worker_;
    worker_fd_->cancel();
  }

  async_simple::coro::Lazy<std::vector<char>> ep_handshake(
      std::vector<char> remote_addr) {
    auto notifier = std::make_unique<async_simple::coro::BinarySemaphore>(0);
    std::vector<char> return_addr;
    auto handler = [this, &remote_addr, &return_addr,
                    &notifier]() -> async_simple::coro::Lazy<> {
      auto remote_ep =
          std::make_unique<ucxpp::endpoint>(&*worker_, remote_addr);
      ucx_server_handle_ep(std::move(remote_ep), &served_ep_count_)
          .via(ucx_executor_)
          .detach();
      return_addr = worker_->get_remote_address();
      co_await notifier->release();
    };
    handler().via(ucx_executor_).detach();
    co_await notifier->acquire();
    co_return return_addr;
  }
};

class coro_ucx_demo_client {
  coro_rpc::coro_rpc_client rpc_client_;
  coro_io::ExecutorWrapper<> *ucx_executor_;
  ucx_environment ucx_env_;
  ucxpp::worker_ptr worker_;
  asio::posix::stream_descriptor *worker_fd_;

 public:
  coro_ucx_demo_client() : ucx_executor_(&rpc_client_.get_executor()) {
    ucx_event_loop(ucx_executor_->context(), *ucx_env_.ctx_,
                   [this](ucxpp::worker_ptr worker,
                          asio::posix::stream_descriptor &worker_fd) {
                     worker_ = worker;
                     worker_fd_ = &worker_fd;
                   })
        .via(ucx_executor_)
        .detach();
  }

  void stop_ucx_worker() {
    delete worker_;
    worker_fd_->cancel();
  }

  async_simple::coro::Lazy<void> run(std::string const &address,
                                     std::string const &port) {
    auto ec = co_await rpc_client_.connect(address, port);
    if (ec) {
      ELOG_ERROR << "connect failed: " << ec.message();
      co_return;
    }

    auto ret = co_await rpc_client_.call<&coro_ucx_demo_service::ep_handshake>(
        worker_->get_remote_address());

    auto notifier = std::make_unique<async_simple::coro::BinarySemaphore>(0);
    auto remote_address = ret.value();
    auto handler = [this, &remote_address,
                    &notifier]() -> async_simple::coro::Lazy<> {
      auto ep = std::make_unique<ucxpp::endpoint>(&*worker_, remote_address);
      co_await ucx_client_handle_ep(std::move(ep));
      co_await notifier->release();
    };

    handler().via(ucx_executor_).detach();
    co_await notifier->acquire();
    stop_ucx_worker();
  }

  async_simple::Future<void> sync_run(std::string const &address,
                                      std::string const &port) {
    async_simple::Promise<void> promise;
    auto future = promise.getFuture();
    run(address, port)
        .via(&rpc_client_.get_executor())
        .start([this, p = std::move(promise)](auto &&res) mutable {
          p.setValue();
        });
    return std::move(future);
  }
};

#endif