#include <cassert>
#include <fstream>
#include <memory>
#include <string_view>
#include <thread>

#include "asio/io_context.hpp"
#include "async_simple/coro/SyncAwait.h"
#include "coro_io/asio_coro_util.hpp"
#ifdef ASIO_HAS_LIB_AIO
#include "coro_io/coro_file.hpp"
#endif

/*
make sure you have install libaio when testing libaio.
yum install libaio-devel
or
apt install libaio-dev
*/
#ifdef ASIO_HAS_LIB_AIO
constexpr size_t FILE_SIZE = 1024;
void create_temp_file() {
  std::ofstream file("test.txt", std::ios::binary);
  file.exceptions(std::ios_base::failbit | std::ios_base::badbit);

  std::string str(FILE_SIZE, 'a');
  file.write(str.data(), str.size());
  file.flush();  // can throw
}

int test_libaio() {
  create_temp_file();
  asio::io_context ioc;
  auto work = std::make_unique<asio::io_context::work>(ioc);
  std::thread thd([&ioc] {
    ioc.run();
  });

  ylt::coro_file file(ioc.get_executor(), "test.txt");
  bool r = file.is_open();
  if (!file.is_open()) {
    return -1;
  }

  void *data = nullptr;
  int ok = posix_memalign(&data, 512, 2048);
  if (ok) {
    fprintf(stderr, "posix_memalign failed: %s\n", strerror(r));
    return -1;
  }

  auto [ec, size] =
      async_simple::coro::syncAwait(file.async_read_some((char *)data, 2048));
  if (ec) {
    std::cout << ec.message() << "\n";
    return -1;
  }
  assert(size == FILE_SIZE);
  std::cout << "read size: " << size << "\n";
  free(data);
  work.reset();
  thd.join();
  return 0;
}
#endif

int main() {
#ifdef ASIO_HAS_LIB_AIO
  test_libaio();
#endif
}