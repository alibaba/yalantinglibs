#include <cassert>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string_view>
#include <thread>

#include "asio/io_context.hpp"
#include "async_simple/coro/SyncAwait.h"
#include "coro_io/asio_coro_util.hpp"
#include "coro_io/coro_file.hpp"
#include "coro_io/io_context_pool.hpp"
#include "doctest.h"

constexpr uint64_t KB = 1024;
constexpr uint64_t MB = 1024 * KB;

std::string create_big_file(std::string filename, size_t file_size, std::string fill_with) {
  std::ofstream file(filename, std::ios::binary);
  file.exceptions(std::ios_base::failbit | std::ios_base::badbit);

  if (!file) {
    std::cout << "create file failed\n";
    return "";
  }
  size_t fill_with_size = fill_with.size();
  if (file_size == 0 || fill_with_size == 0) {
    return "";
  }
  std::string str;
  int cnt = file_size / fill_with_size;
  int remain = file_size % fill_with_size;
  for (int i = 0; i < cnt; i++) {
    file.write(fill_with.data(), fill_with_size);
    str += fill_with;
  }
  if (remain > 0) {
    file.write(fill_with.data(), remain);
    str += fill_with.substr(remain);
  }
  file.flush();  // can throw
  return str;
}

void create_small_file(std::string filename, std::string file_content) {
  std::ofstream file(filename, std::ios::binary);
  file.exceptions(std::ios_base::failbit | std::ios_base::badbit);

  if (!file) {
    std::cout << "create file failed\n";
    return;
  }
  if (file_content.size() == 0) {
    return;
  }
  file.write(file_content.data(), file_content.size());

  file.flush();  // can throw
}

#ifdef ENABLE_FILE_IO_URING

TEST_CASE("test coro http bearer token auth request") {

}

#else

TEST_CASE("small_file_read_test") {
  std::string filename = "small_file_read_test.txt";
  std::string file_content = "small_file_read_test";
  create_small_file(filename, file_content);
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

  char buf[512]{};
  auto [ec, read_size] =
        async_simple::coro::syncAwait(file.async_read(buf, 512));
  if (ec) {
      std::cout << ec.message() << "\n";
  }
  auto read_content = std::string_view(buf, read_size);
  std::cout << read_size << "\n";
  std::cout << read_content << "\n";
  CHECK(read_size == file_content.size());
  CHECK(read_content == file_content);
  work.reset();
  thd.join();

}
TEST_CASE("empty_file_read_test") {
  std::string filename = "empty_file_read_test.txt";
  std::string file_content = "";
  create_small_file(filename, file_content);
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

  char buf[512]{};
  auto [ec, read_size] =
        async_simple::coro::syncAwait(file.async_read(buf, 512));
  if (ec) {
      std::cout << ec.message() << "\n";
  }
  auto read_content = std::string_view(buf, read_size);
  std::cout << read_size << "\n";
  std::cout << read_content << "\n";
  CHECK(read_size == file_content.size());
  CHECK(read_content == file_content);
  work.reset();
  thd.join();

}
TEST_CASE("big_file_read_test") {
  std::string filename = "big_file_read_test.txt";
  std::string file_with = "abc";
  uint64_t file_size = 100 * MB;
  auto file_content = std::string_view(std::move(create_big_file(filename, file_size, file_with)));
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

  std::vector<char> buf(file_size);
  auto [ec, read_size] =
        async_simple::coro::syncAwait(file.async_read(buf.data(), file_size));
  if (ec) {
      std::cout << ec.message() << "\n";
  }
  auto read_content = std::string_view(buf.data(), read_size);
  std::cout << read_size << "\n";
//   std::cout << read_content << "\n";
  CHECK(read_size == file_size);
  CHECK(read_content == file_content);
  work.reset();
  thd.join();

}
#endif