#include <cassert>
#include <fstream>
#include <memory>
#include <string_view>
#include <thread>

#include "asio/io_context.hpp"
#include "async_simple/coro/SyncAwait.h"
#include "coro_io/asio_coro_util.hpp"
#if defined(ASIO_HAS_LIB_AIO) || defined(ASIO_HAS_IO_URING)
#include "coro_io/coro_file.hpp"
#endif

/*
make sure you have install libaio when testing libaio.
yum install libaio-devel
or
apt install libaio-dev
*/
#if defined(ASIO_HAS_LIB_AIO) || defined(ASIO_HAS_IO_URING)
void create_temp_file(std::string filename, size_t size) {
  std::ofstream file(filename, std::ios::binary);
  file.exceptions(std::ios_base::failbit | std::ios_base::badbit);

  {
    std::string str(size, 'a');
    file.write(str.data(), str.size());
  }
  {
    std::string str(size, 'b');
    file.write(str.data(), str.size());
  }
  {
    std::string str(42, 'c');
    file.write(str.data(), str.size());
  }

  file.flush();  // can throw
}

void test_read_file() {
  std::string filename = "test1.txt";
  create_temp_file("test1.txt", 1024);
  asio::io_context ioc;
  auto work = std::make_unique<asio::io_context::work>(ioc);
  std::thread thd([&ioc] {
    ioc.run();
  });

  ylt::coro_file file(ioc.get_executor(), filename);
  bool r = file.is_open();
  if (!file.is_open()) {
    return;
  }

  while (!file.eof()) {
    auto [ec, buf] = async_simple::coro::syncAwait(file.async_read_some());
    if (ec) {
      std::cout << ec.message() << "\n";
      break;
    }

    std::cout << buf.size() << "\n";
    std::cout << buf << "\n";
  }

  work.reset();
  thd.join();
}
#endif

int main() {
#if defined(ASIO_HAS_LIB_AIO) || defined(ASIO_HAS_IO_URING)
  test_read_file();
#endif
}