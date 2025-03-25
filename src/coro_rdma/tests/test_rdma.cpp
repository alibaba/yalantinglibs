#include "../rdma_test_common.hpp"

int main(int argc, char** argv) {
  int pid = ::fork();
  if (pid < 0) {
    ::perror("fork");
    return -1;
  }
  try {
    if (pid == 0) {
      coro_rdma_demo_client client;
      ::sleep(1);
      async_simple::coro::syncAwait(client.run("127.0.0.1", "55555"));
    }
    else {
      coro_rpc::coro_rpc_server server(2, 55555, "0.0.0.0");
      coro_rdma_demo_service service(
          server.get_io_context_pool().get_executor());
      server.register_handler<&coro_rdma_demo_service::qp_handshake>(&service);
      std::jthread watcher([&server, &service]() {
        while (service.served_qp_count_ == 0) {
          std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        service.stop_rdma_cq_worker();
        server.stop();
      });
      auto ec = server.start();
      if (ec) {
        ELOG_INFO << "server start failed, ec: " << ec;
      }
      int status;
      ::wait(&status);
    }
  } catch (std::exception const& e) {
    ELOG_ERROR << "Caught exception: " << std::string(e.what());
  }

  return 0;
}