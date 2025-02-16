#include "../rdma_test_common.hpp"

int main(int argc, char *argv[]) {
  // For server run ./example [port]
  // For client run ./example [ip] [port]
  auto socket_ctx = std::make_unique<asio::io_context>();
  auto executor_wrapper =
      std::make_unique<coro_io::ExecutorWrapper<>>(socket_ctx->get_executor());
  auto device = std::make_unique<rdmapp::device>();
  auto pd = std::make_unique<rdmapp::pd>(&*device);

  auto channel = std::make_unique<rdmapp::comp_channel>(&*device);
  channel->set_non_blocking();
  auto cq = std::make_unique<rdmapp::cq>(&*device, 128, &*channel);
  cq->request_notify();

  process_rdma_cq(*socket_ctx, &*channel).via(&*executor_wrapper).detach();
  std::jthread worker_thread([&socket_ctx] {
    socket_ctx->run();
  });

  if (argc == 2) {
    std::cout << "server mode, press Control+C to exit" << std::endl;
    std::string port_str = argv[1];
    rdma_qp_server server(*socket_ctx, &*pd, &*cq, std::stoi(port_str));
    auto ec = server.listen();
    if (ec) {
      std::cerr << "listen failed: " << ec.message() << std::endl;
      return -1;
    }
    async_simple::coro::syncAwait(server.loop());
  }
  else if (argc == 3) {
    std::cout << "client mode, connecting to server" << std::endl;
    std::string ip = argv[1];
    std::string port_str = argv[2];
    rdma_qp_client client(*socket_ctx, &*pd, &*cq, std::stoi(port_str), ip);
    // TODO: fix memory leak
    async_simple::coro::syncAwait(client.run());
    socket_ctx->stop();
  }
  else {
    std::cerr << "server usage: " << argv[0] << " [port]" << std::endl;
    std::cerr << "client usage: " << argv[0] << " [ip] [port]" << std::endl;
  }
  return 0;
}