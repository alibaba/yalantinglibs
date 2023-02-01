#include <coro_rpc/coro_rpc_server.hpp>
#include <iostream>

#include "../rpc_service/rpc_service.h"



int main() {
  coro_rpc::coro_rpc_server server(4, 9000);

  server.regist_handler<echo, upload, upload_file, download_file>();

  dummy d{};
  server.regist_handler<&dummy::echo>(&d);

  [[maybe_unused]] auto ret = server.start();
  assert(ret == std::errc{});
  return 0;
}
