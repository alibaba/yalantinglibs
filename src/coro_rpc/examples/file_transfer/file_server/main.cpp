#include <coro_rpc/coro_rpc_server.hpp>
#include <iostream>

#include "../rpc_service/rpc_service.h"
int main() {
  coro_rpc::register_handler<echo, upload, upload_file, download_file>();

  dummy d{};
  coro_rpc::register_one_handler<&dummy::echo>(&d);

  coro_rpc::coro_rpc_server server(4, 9000);
  auto ret = server.start();
  assert(ret == std::errc{});
  return 0;
}
