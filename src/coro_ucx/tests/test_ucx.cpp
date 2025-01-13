#include "../ucx_test_common.hpp"

int main(int argc, char *argv[]) {
  int pid = ::fork();
  if (pid < 0) {
    ::perror("fork");
    return -1;
  }

  if (pid == 0) {
    ::sleep(1);
    coro_ucx_demo_client client;
    client.sync_run("127.0.0.1", "44444").get();
  }
  else {
    coro_rpc::coro_rpc_server server(2, 44444, "0.0.0.0");
    coro_ucx_demo_service service(server.get_io_context_pool().get_executor());
    server.register_handler<&coro_ucx_demo_service::ep_handshake>(&service);
    std::jthread watcher([&server, &service]() {
      while (service.served_ep_count_ == 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      }
      service.stop_ucx_worker();
      server.stop();
    });
    auto ec = server.start();
    if (ec) {
      ELOG_INFO << "server start failed, ec: " << ec;
    }
    int status;
    ::wait(&status);
  }

  return 0;
}