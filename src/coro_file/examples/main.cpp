#include <cassert>
#include <fstream>
#include <memory>
#include <string_view>
#include <thread>

#include "asio/io_context.hpp"
#include "async_simple/coro/SyncAwait.h"
#include "coro_io/asio_coro_util.hpp"
#if defined(ENABLE_FILE_IO_URING)
#include "coro_io/coro_file.hpp"
#endif

/*
make sure you have install libaio when testing libaio.
yum install libaio-devel
or
apt install libaio-dev
*/
#if defined(ENABLE_FILE_IO_URING)
void create_temp_file(std::string filename, size_t size) {
  std::ofstream file(filename, std::ios::binary);
  file.exceptions(std::ios_base::failbit | std::ios_base::badbit);

  if (!file) {
    std::cout << "create file failed\n";
    return;
  }

  if (size == 0) {
    return;
  }

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

  char buf[1024]{};

  while (!file.eof()) {
    auto [ec, read_size] =
        async_simple::coro::syncAwait(file.async_read(buf, 1024));
    if (ec) {
      std::cout << ec.message() << "\n";
      break;
    }

    std::cout << read_size << "\n";
    std::cout << std::string_view(buf, read_size) << "\n";
  }

  work.reset();
  thd.join();
}

void test_write_and_read_file() {
  std::string filename = "test.txt";
  create_temp_file(filename, 0);

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

  std::string str = "test async write";

  auto ec =
      async_simple::coro::syncAwait(file.async_write(str.data(), str.size()));
  if (ec) {
    std::cout << ec.message() << "\n";
  }

  std::string str1 = "another test async write";
  ec =
      async_simple::coro::syncAwait(file.async_write(str1.data(), str1.size()));
  if (ec) {
    std::cout << ec.message() << "\n";
  }

  ylt::coro_file file1(ioc.get_executor(), filename);
  r = file1.is_open();
  if (!file1.is_open()) {
    return;
  }

  char buf[2048]{};
  while (!file1.eof()) {
    auto [read_ec, read_size] =
        async_simple::coro::syncAwait(file1.async_read(buf, 2048));
    if (read_ec) {
      std::cout << read_ec.message() << "\n";
      return;
    }

    std::cout << read_size << "\n";
    std::cout << std::string_view(buf, read_size) << "\n";
  }

  work.reset();
  thd.join();
}
#endif

int main() {
#if defined(ENABLE_FILE_IO_URING)
  test_read_file();
  test_write_and_read_file();
#endif
}