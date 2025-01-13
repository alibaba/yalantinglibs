#include "../rdma_test_common.hpp"
int main(int argc, char **argv) {
  int pid = ::fork();
  auto ctx = std::make_unique<asio::io_context>();
  auto executor_wrapper =
      std::make_unique<coro_io::ExecutorWrapper<>>(ctx->get_executor());

  auto device = std::make_unique<rdmapp::device>();

  auto pd = std::make_unique<rdmapp::pd>(&*device);

  auto channel = std::make_unique<rdmapp::comp_channel>(&*device);
  channel->set_non_blocking();

  auto cq = std::make_unique<rdmapp::cq>(&*device, 128, &*channel);
  cq->request_notify();

  process_rdma_cq(*ctx, &*channel).via(&*executor_wrapper).detach();

  std::thread loop{[&ctx]() {
    ctx->run();
  }};

  if (pid == 0) {
    ::sleep(1);
    rdma_qp_client client(*ctx, &*pd, &*cq, 12345, "127.0.0.1");
    async_simple::coro::syncAwait(client.run());
  }
  else {
    rdma_qp_server server(*ctx, &*pd, &*cq, 12345);
    auto ec = server.listen();
    if (ec) {
      std::cerr << "listen failed: " << ec.message() << std::endl;
    }
    else {
      async_simple::coro::syncAwait(server.run());
    }
    int status;
    ::wait(&status);
  }
  channel.reset();
  ctx->stop();
  loop.join();
}