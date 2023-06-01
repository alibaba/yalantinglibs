#include <algorithm>
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

namespace fs = std::filesystem;

constexpr uint64_t KB = 1024;
constexpr uint64_t MB = 1024 * KB;

std::vector<char> create_filled_vec(std::string fill_with, size_t size) {
  std::vector<char> ret(size);
  size_t fill_with_size = fill_with.size();
  int cnt = size / fill_with_size;
  int remain = size % fill_with_size;
  for (int i = 0; i < cnt; i++) {
    memcpy(ret.data() + i * fill_with_size, fill_with.data(), fill_with_size);
  }
  if (remain > 0) {
    memcpy(ret.data() + size - remain, fill_with.data(), remain);
  }
  return ret;
}
std::vector<char> create_big_file(std::string filename, size_t file_size,
                                  std::string fill_with) {
  std::ofstream file(filename, std::ios::binary);
  file.exceptions(std::ios_base::failbit | std::ios_base::badbit);

  if (!file) {
    std::cout << "create file failed\n";
    return std::vector<char>{};
  }
  size_t fill_with_size = fill_with.size();
  if (file_size == 0 || fill_with_size == 0) {
    return std::vector<char>{};
  }
  std::vector<char> ret{create_filled_vec(fill_with, file_size)};
  file.write(ret.data(), file_size);
  file.flush();  // can throw
  return ret;
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
  CHECK(file.is_open());

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
  fs::remove(fs::path(filename));
}
TEST_CASE("big_file_read_test") {
  std::string filename = "big_file_read_test.txt";
  std::string file_with = "abc";
  uint64_t file_size = 100 * MB;
  std::vector<char> file_content{
      create_big_file(filename, file_size, file_with)};
  asio::io_context ioc;
  auto work = std::make_unique<asio::io_context::work>(ioc);
  std::thread thd([&ioc] {
    ioc.run();
  });

  ylt::coro_file file(ioc.get_executor(), filename);
  CHECK(file.is_open());

  std::vector<char> read_content(file_size);
  auto [ec, read_size] = async_simple::coro::syncAwait(
      file.async_read(read_content.data(), file_size));
  if (ec) {
    std::cout << ec.message() << "\n";
  }
  std::cout << read_size << "\n";
  //   std::cout << read_content << "\n";
  CHECK(true == std::equal(file_content.begin(), file_content.end(),
                           read_content.begin()));
  work.reset();
  thd.join();
  fs::remove(fs::path(filename));
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
  CHECK(file.is_open());

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
  fs::remove(fs::path(filename));
}
TEST_CASE("small_file_write_test") {
  std::string filename = "small_file_write_test.txt";
  asio::io_context ioc;
  auto work = std::make_unique<asio::io_context::work>(ioc);
  std::thread thd([&ioc] {
    ioc.run();
  });

  ylt::coro_file file(ioc.get_executor(), filename);
  CHECK(file.is_open());

  char buf[512]{};

  std::string file_content_0 = "small_file_write_test_0";

  auto ec = async_simple::coro::syncAwait(
      file.async_write(file_content_0.data(), file_content_0.size()));
  if (ec) {
    std::cout << ec.message() << "\n";
  }

  std::ifstream is(filename, std::ios::binary);
  if (!is.is_open()) {
    std::cout << "Failed to open file: " << filename << "\n";
    return;
  }
  is.seekg(0, std::ios::end);
  auto size = is.tellg();
  is.seekg(0, std::ios::beg);
  is.read(buf, size);
  CHECK(size == file_content_0.size());
  is.close();
  auto read_content = std::string_view(buf, size);
  std::cout << read_content << "\n";
  CHECK(read_content == file_content_0);

  std::string file_content_1 = "small_file_write_test_1";

  ec = async_simple::coro::syncAwait(
      file.async_write(file_content_1.data(), file_content_1.size()));
  if (ec) {
    std::cout << ec.message() << "\n";
  }
  is.open(filename, std::ios::binary);
  if (!is.is_open()) {
    std::cout << "Failed to open file: " << filename << "\n";
    return;
  }
  is.seekg(0, std::ios::end);
  size = is.tellg();
  is.seekg(0, std::ios::beg);
  CHECK(size == (file_content_0.size() + file_content_1.size()));
  is.read(buf, size);
  is.close();
  read_content = std::string_view(buf, size);
  std::cout << read_content << "\n";
  CHECK(read_content == (file_content_0 + file_content_1));

  work.reset();
  thd.join();
  fs::remove(fs::path(filename));
}
TEST_CASE("big_file_write_test") {
  std::string filename = "big_file_write_test.txt";
  size_t file_size = 100 * MB;
  asio::io_context ioc;
  auto work = std::make_unique<asio::io_context::work>(ioc);
  std::thread thd([&ioc] {
    ioc.run();
  });

  ylt::coro_file file(ioc.get_executor(), filename);
  CHECK(file.is_open());

  auto file_content = create_filled_vec("big_file_write_test", file_size);

  auto ec = async_simple::coro::syncAwait(
      file.async_write(file_content.data(), file_content.size()));
  if (ec) {
    std::cout << ec.message() << "\n";
  }

  std::ifstream is(filename, std::ios::binary);
  if (!is.is_open()) {
    std::cout << "Failed to open file: " << filename << "\n";
    return;
  }
  is.seekg(0, std::ios::end);
  auto size = is.tellg();
  is.seekg(0, std::ios::beg);
  CHECK(size == file_size);
  std::vector<char> read_content(size);
  is.read(read_content.data(), size);
  is.close();
  CHECK(true == std::equal(file_content.begin(), file_content.end(),
                           read_content.begin()));

  work.reset();
  thd.join();
  fs::remove(fs::path(filename));
}
TEST_CASE("empty_file_write_test") {
  std::string filename = "empty_file_write_test.txt";
  asio::io_context ioc;
  auto work = std::make_unique<asio::io_context::work>(ioc);
  std::thread thd([&ioc] {
    ioc.run();
  });

  ylt::coro_file file(ioc.get_executor(), filename);
  CHECK(file.is_open());

  char buf[512]{};

  std::string file_content_0 = "small_file_write_test_0";

  auto ec =
      async_simple::coro::syncAwait(file.async_write(file_content_0.data(), 0));
  if (ec) {
    std::cout << ec.message() << "\n";
  }

  std::ifstream is(filename, std::ios::binary);
  if (!is.is_open()) {
    std::cout << "Failed to open file: " << filename << "\n";
    return;
  }
  is.seekg(0, std::ios::end);
  auto size = is.tellg();
  CHECK(size == 0);
  is.close();
  work.reset();
  thd.join();
  fs::remove(fs::path(filename));
}
