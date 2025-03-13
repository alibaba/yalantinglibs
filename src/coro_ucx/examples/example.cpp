#include "../ucx_test_common.hpp"

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