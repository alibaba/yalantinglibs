#include <async_simple/coro/Lazy.h>
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

constexpr ucp_tag_t kTestTag = 0xFD709394UL;
constexpr ucp_tag_t kBellTag = 0xbe11be11UL;

static std::atomic<bool> stopped = false;

static inline async_simple::coro::Lazy<void> ucx_event_loop(
    asio::io_context &ctx, ucxpp::worker_ptr worker) {
  asio::posix::stream_descriptor worker_fd(ctx.get_executor(),
                                           worker->event_fd());
  while (!ctx.stopped()) {
    coro_io::callback_awaitor<std::error_code> awaitor;
    auto ec = co_await awaitor.await_resume([&worker_fd](auto handler) {
      worker_fd.async_wait(asio::posix::stream_descriptor::wait_read,
                           [handler](const auto &ec) mutable {
                             handler.set_value_then_resume(ec);
                           });
    });

    if (ec) {
      std::cerr << "failed to wait: " << ec.message() << std::endl;
      co_return;
    }

    do {
      while (worker->progress());
    } while (!worker->arm());
  }
}

async_simple::coro::Lazy<ucxpp::endpoint_ptr> recv_ep(
    ucxpp::worker_ptr worker, asio::ip::tcp::socket &socket) {
  std::array<uint8_t, sizeof(size_t)> address_length_buf;
  {
    auto [ec, size] =
        co_await coro_io::async_read(socket, asio::buffer(address_length_buf));
    if (ec) {
      std::cerr << ec.message() << std::endl;
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
      std::cerr << ec.message() << std::endl;
      throw std::runtime_error("read address failed");
    }
  }

  auto remote_addr = ucxpp::remote_address(std::move(address_buf));
  co_return new ucxpp::endpoint(worker, remote_addr);
}

async_simple::coro::Lazy<void> send_ep(
    ucxpp::worker::local_address const &address,
    asio::ip::tcp::socket &socket) {
  auto [ec, size] =
      co_await coro_io::async_write(socket, asio::buffer(address.serialize()));
}

async_simple::coro::Lazy<std::pair<uint64_t, ucxpp::remote_memory_handle>>
receive_mr(ucxpp::endpoint_ptr ep) {
  using std::cout;
  using std::endl;
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

async_simple::coro::Lazy<void> ucx_client_handle_ep(ucxpp::endpoint_ptr ep) {
  ep->print();
  char buffer[6];

  /* Tag Send/Recv */
  auto [n, sender_tag] =
      co_await ep->worker()->tag_recv(buffer, sizeof(buffer), kTestTag);
  std::cout << "Received " << n << " bytes from " << std::hex << sender_tag
            << std::dec << ": " << buffer << std::endl;
  std::copy_n("world", 6, buffer);
  co_await ep->tag_send(buffer, sizeof(buffer), kTestTag);

  /* Stream Send/Recv */
  n = co_await ep->stream_recv(buffer, sizeof(buffer));
  std::cout << "Received " << n << " bytes: " << buffer << std::endl;
  std::copy_n("world", 6, buffer);
  co_await ep->stream_send(buffer, sizeof(buffer));

  /* RMA Get/Put */
  auto local_mr = ucxpp::local_memory_handle::register_mem(
      ep->worker()->context(), buffer, sizeof(buffer));
  auto [remote_addr, remote_mr] = co_await receive_mr(ep);
  std::cout << "Remote addr: 0x" << std::hex << remote_addr << std::dec
            << std::endl;
  co_await remote_mr.get(buffer, sizeof(buffer), remote_addr);
  std::cout << "Read from server: " << buffer << std::endl;
  std::copy_n("world", 6, buffer);
  co_await remote_mr.put(buffer, sizeof(buffer), remote_addr);
  std::cout << "Wrote to server: " << buffer << std::endl;
  size_t bell;
  co_await ep->tag_send(&bell, sizeof(bell), kBellTag);

  /* Atomic */
  uint64_t local_value = 1;
  uint64_t reply_value = 0;
  auto [atomic_raddr, atomic_mr] = co_await receive_mr(ep);

  /* Fetch and Add */
  co_await atomic_mr.atomic_fetch_add(atomic_raddr, local_value, reply_value);
  std::cout << "Fetched and added on server: " << reply_value << std::endl;
  co_await ep->tag_send(&bell, sizeof(bell), kBellTag);
  co_await ep->worker()->tag_recv(&bell, sizeof(bell), kBellTag);

  /* Compare and Swap */
  local_value = reply_value + local_value;
  reply_value = 456;
  co_await atomic_mr.atomic_compare_swap(atomic_raddr, local_value,
                                         reply_value);
  std::cout << "Compared and swapped on server: " << reply_value << std::endl;
  co_await ep->tag_send(&bell, sizeof(bell), kBellTag);
  co_await ep->worker()->tag_recv(&bell, sizeof(bell), kBellTag);

  /* Swap */
  local_value = 123;
  co_await atomic_mr.atomic_swap(atomic_raddr, local_value, reply_value);
  std::cout << "Swapped on server: " << reply_value << std::endl;
  co_await ep->tag_send(&bell, sizeof(bell), kBellTag);
  co_await ep->worker()->tag_recv(&bell, sizeof(bell), kBellTag);

  /* Fetch and And */
  local_value = 0xF;
  co_await atomic_mr.atomic_fetch_and(atomic_raddr, local_value, reply_value);
  std::cout << "Fetched and anded on server: " << reply_value << std::endl;
  co_await ep->tag_send(&bell, sizeof(bell), kBellTag);
  co_await ep->worker()->tag_recv(&bell, sizeof(bell), kBellTag);

  /* Fetch and Or */
  local_value = 0xF;
  co_await atomic_mr.atomic_fetch_or(atomic_raddr, local_value, reply_value);
  std::cout << "Fetched and ored on server: " << reply_value << std::endl;
  co_await ep->tag_send(&bell, sizeof(bell), kBellTag);
  co_await ep->worker()->tag_recv(&bell, sizeof(bell), kBellTag);

  /* Fetch and Xor */
  local_value = 0xF;
  co_await atomic_mr.atomic_fetch_xor(atomic_raddr, local_value, reply_value);
  std::cout << "Fetched and xored on server: " << reply_value << std::endl;
  co_await ep->tag_send(&bell, sizeof(bell), kBellTag);
  co_await ep->worker()->tag_recv(&bell, sizeof(bell), kBellTag);

  co_await ep->flush();
  co_await ep->close();

  co_return;
}

async_simple::coro::Lazy<void> send_mr(
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

async_simple::coro::Lazy<void> ucx_server_handle_ep(ucxpp::endpoint_ptr ep) {
  ep->print();
  char buffer[6] = "Hello";

  /* Tag Send/Recv */
  co_await ep->tag_send(buffer, sizeof(buffer), kTestTag);
  auto [n, sender_tag] =
      co_await ep->worker()->tag_recv(buffer, sizeof(buffer), kTestTag);
  std::cout << "Received " << n << " bytes from " << std::hex << sender_tag
            << std::dec << ": " << buffer << std::endl;

  /* Stream Send/Recv */
  std::copy_n("Hello", 6, buffer);
  co_await ep->stream_send(buffer, sizeof(buffer));
  n = co_await ep->stream_recv(buffer, sizeof(buffer));
  std::cout << "Received " << n << " bytes: " << buffer << std::endl;

  /* RMA Get/Put */
  std::copy_n("Hello", 6, buffer);
  auto local_mr = ucxpp::local_memory_handle::register_mem(
      ep->worker()->context(), buffer, sizeof(buffer));
  co_await send_mr(ep, buffer, local_mr);

  size_t bell;
  co_await ep->worker()->tag_recv(&bell, sizeof(bell), kBellTag);
  std::cout << "Written by client: " << buffer << std::endl;

  /* Atomic */
  uint64_t value = 42;
  auto atomic_mr = ucxpp::local_memory_handle::register_mem(
      ep->worker()->context(), &value, sizeof(value));
  co_await send_mr(ep, &value, atomic_mr);

  co_await ep->worker()->tag_recv(&bell, sizeof(bell), kBellTag);
  std::cout << "Fetched and added by client: " << value << std::endl;
  co_await ep->tag_send(&bell, sizeof(bell), kBellTag);

  co_await ep->worker()->tag_recv(&bell, sizeof(bell), kBellTag);
  std::cout << "Compared and Swapped by client: " << value << std::endl;
  co_await ep->tag_send(&bell, sizeof(bell), kBellTag);

  co_await ep->worker()->tag_recv(&bell, sizeof(bell), kBellTag);
  std::cout << "Swapped by client: " << value << std::endl;
  co_await ep->tag_send(&bell, sizeof(bell), kBellTag);

  co_await ep->worker()->tag_recv(&bell, sizeof(bell), kBellTag);
  std::cout << "Fetched and Anded by client: " << value << std::endl;
  co_await ep->tag_send(&bell, sizeof(bell), kBellTag);

  co_await ep->worker()->tag_recv(&bell, sizeof(bell), kBellTag);
  std::cout << "Fetched and Ored by client: " << value << std::endl;
  co_await ep->tag_send(&bell, sizeof(bell), kBellTag);

  co_await ep->worker()->tag_recv(&bell, sizeof(bell), kBellTag);
  std::cout << "Fetched and Xored by client: " << value << std::endl;
  co_await ep->tag_send(&bell, sizeof(bell), kBellTag);

  co_await ep->flush();
  co_await ep->close();

  co_return;
}

class ucx_ep_client {
  asio::io_context *ctx_;
  coro_io::ExecutorWrapper<> executor_;
  uint16_t port_;
  std::string address_;
  std::error_code errc_ = {};
  asio::ip::tcp::socket socket_;
  ucxpp::worker_ptr worker_;

 public:
  ucx_ep_client(asio::io_context &ctx, ucxpp::worker_ptr worker,
                unsigned short port, std::string address = "0.0.0.0")
      : ctx_(&ctx),
        worker_(worker),
        executor_(ctx.get_executor()),
        port_(port),
        address_(address),
        socket_(ctx) {}

  async_simple::coro::Lazy<void> handle_server_connection(
      asio::ip::tcp::socket &socket) {
    co_await send_ep(worker_->get_address(), socket);
    std::cerr << "send ep success" << std::endl;
    auto remote_ep = co_await recv_ep(worker_, socket);
    std::cerr << "recv ep success" << std::endl;
    co_await ucx_client_handle_ep(remote_ep);
    delete remote_ep;
  }

  async_simple::coro::Lazy<void> run() {
    auto ec = co_await coro_io::async_connect(&executor_, socket_, address_,
                                              std::to_string(port_));
    if (ec) {
      throw std::system_error(ec, "connect failed");
    }
    co_await handle_server_connection(socket_);
    ctx_->stop();
  }
};

class ucx_ep_server {
  asio::io_context *ctx_;
  coro_io::ExecutorWrapper<> executor_;
  uint16_t port_;
  std::string address_;
  std::error_code errc_ = {};
  asio::ip::tcp::acceptor acceptor_;
  std::promise<void> acceptor_close_waiter_;
  ucxpp::worker_ptr worker_;

 public:
  ucx_ep_server(asio::io_context &ctx, ucxpp::worker_ptr worker,
                unsigned short port, std::string address = "0.0.0.0")
      : ctx_(&ctx),
        worker_(worker),
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

  async_simple::coro::Lazy<void> handle_client_connection(
      asio::ip::tcp::socket socket) {
    auto remote_ep = co_await recv_ep(worker_, socket);
    std::cerr << "recv ep success" << std::endl;
    co_await send_ep(worker_->get_address(), socket);
    std::cerr << "send ep success" << std::endl;
    co_await ucx_server_handle_ep(remote_ep);
    delete remote_ep;
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

int main(int argc, char *argv[]) {
  // For server run ./example [port]
  // For client run ./example [ip] [port]
  auto socket_ctx = std::make_unique<asio::io_context>();
  auto executor_wrapper =
      std::make_unique<coro_io::ExecutorWrapper<>>(socket_ctx->get_executor());
  auto ucx_ctx = ucxpp::context::builder()
                     .enable_stream()
                     .enable_tag()
                     .enable_wakeup()
                     .enable_rma()
                     .enable_amo64()
                     .build();
  auto worker = std::make_unique<ucxpp::worker>(ucx_ctx);

  ucx_event_loop(*socket_ctx, &*worker).via(&*executor_wrapper).detach();

  if (argc == 2) {
    std::cout << "server mode, press Control+C to exit" << std::endl;
    std::string port_str = argv[1];
    ucx_ep_server server(*socket_ctx, &*worker, std::stoi(port_str));
    auto ec = server.listen();
    if (ec) {
      std::cerr << "listen failed: " << ec.message() << std::endl;
      return -1;
    }
    server.loop().via(&*executor_wrapper).detach();
    socket_ctx->run();
  }
  else if (argc == 3) {
    std::cout << "client mode, connecting to server" << std::endl;
    std::string ip = argv[1];
    std::string port_str = argv[2];
    ucx_ep_client client(*socket_ctx, &*worker, std::stoi(port_str), ip);
    // TODO: fix memory leak
    client.run().via(&*executor_wrapper).detach();
    socket_ctx->run();
  }
  else {
    std::cerr << "server usage: " << argv[0] << " [port]" << std::endl;
    std::cerr << "client usage: " << argv[0] << " [ip] [port]" << std::endl;
  }

  return 0;
}