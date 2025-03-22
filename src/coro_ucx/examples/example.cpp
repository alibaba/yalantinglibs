#include "../ucx_test_common.hpp"

int main(int argc, char *argv[]) {
  // For server run ./example [port]
  // For client run ./example [ip] [port]
  if (argc == 2) {
    std::cout << "server mode, press Control+C to exit" << std::endl;
    std::string port_str = argv[1];
    coro_rpc::coro_rpc_server server(2, std::stoi(port_str), "0.0.0.0");
    coro_ucx_demo_service service(server.get_io_context_pool().get_executor());
    server.register_handler<&coro_ucx_demo_service::ep_handshake>(&service);
    auto ec = server.start();
    if (ec) {
      ELOG_INFO << "start server failed: " << ec;
      return -1;
    }
  }
  else if (argc == 3) {
    std::cout << "client mode, connecting to server" << std::endl;
    std::string ip = argv[1];
    std::string port_str = argv[2];

    coro_ucx_demo_client client;
    client.sync_run(ip, port_str).get();
  }
  else {
    std::cerr << "server usage: " << argv[0] << " [port]" << std::endl;
    std::cerr << "client usage: " << argv[0] << " [ip] [port]" << std::endl;
  }

  return 0;
}