#include "../ucx_test_common.hpp"

int main(int argc, char *argv[]) {
  int pid = ::fork();
  if (pid < 0) {
    ::perror("fork");
    return -1;
  }

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

  if (pid == 0) {
    ::sleep(1);
    ucx_ep_client client(*socket_ctx, &*worker, 54321, "127.0.0.1");

    client.run_and_stop().via(&*executor_wrapper).detach();
    socket_ctx->run();
  }
  else {
    ucx_ep_server server(*socket_ctx, &*worker, 54321, "127.0.0.1");
    auto ec = server.listen();
    if (ec) {
      std::cerr << "listen failed: " << ec.message() << std::endl;
      return -1;
    }
    server.run_and_stop().via(&*executor_wrapper).detach();
    socket_ctx->run();
  }

  return 0;
}