#include <async_simple/coro/Collect.h>
#include <async_simple/coro/SyncAwait.h>
#include <doctest.h>

#include <asio/io_context.hpp>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string_view>
#include <system_error>
#include <thread>
#include <ylt/coro_io/coro_file.hpp>
#include <ylt/coro_io/coro_io.hpp>
#include <ylt/coro_io/io_context_pool.hpp>

namespace fs = std::filesystem;

constexpr uint64_t KB = 1024;
constexpr uint64_t MB = 1024 * KB;
constexpr uint64_t block_size = 4 * KB;

std::vector<char> create_filled_vec(std::string fill_with,
                                    size_t size = block_size) {
  if (fill_with.empty() || size == 0)
    return std::vector<char>{};
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
void create_file(std::string filename, size_t file_size,
                 const std::vector<char>& fill_with_vec) {
  std::ofstream file(filename, std::ios::binary);
  file.exceptions(std::ios_base::failbit | std::ios_base::badbit);

  if (!file) {
    std::cout << "create file failed\n";
    return;
  }
  size_t fill_with_size = fill_with_vec.size();
  if (file_size == 0 || fill_with_size == 0) {
    return;
  }
  int cnt = file_size / block_size;
  int remain = file_size - block_size * cnt;
  for (size_t i = 0; i < cnt; i++) {
    file.write(fill_with_vec.data(), block_size);
  }
  if (remain > 0) {
    file.write(fill_with_vec.data(), remain);
  }
  file.flush();  // can throw
  return;
}

void create_files(const std::vector<std::string>& files, size_t file_size) {
  std::string content(file_size, 'A');
  for (auto& filename : files) {
    std::ofstream out(filename, std::ios::binary);
    out.write(content.data(), content.size());
  }
}

TEST_CASE("validate corofile") {
  std::string filename = "validate.tmp";
  create_files({filename}, 190);
  {
    coro_io::coro_file file{};
    async_simple::coro::syncAwait(file.async_open(
        filename.data(), coro_io::flags::read_only, coro_io::read_type::pread));
    CHECK(file.is_open());

    char buf[100];
    std::error_code ec;
    size_t size;
    std::tie(ec, size) =
        async_simple::coro::syncAwait(file.async_read(buf, 10));
    CHECK(ec == std::make_error_code(std::errc::bad_file_descriptor));
    CHECK(size == 0);

    auto write_ec = async_simple::coro::syncAwait(file.async_write(buf, 10));
    CHECK(write_ec == std::make_error_code(std::errc::bad_file_descriptor));
  }
#if defined(YLT_ENABLE_FILE_IO_URING)
  {
    coro_io::coro_file file{};
    async_simple::coro::syncAwait(
        file.async_open(filename.data(), coro_io::flags::read_only,
                        coro_io::read_type::uring_random));
    CHECK(file.is_open());

    char buf[100];
    std::error_code ec;
    size_t size;
    std::tie(ec, size) =
        async_simple::coro::syncAwait(file.async_read(buf, 10));
    CHECK(ec == std::make_error_code(std::errc::bad_file_descriptor));
    CHECK(size == 0);

    ec = async_simple::coro::syncAwait(file.async_write(buf, 10));
    CHECK(ec == std::make_error_code(std::errc::bad_file_descriptor));
  }

  {
    coro_io::coro_file file{};
    async_simple::coro::syncAwait(file.async_open(
        filename.data(), coro_io::flags::read_only, coro_io::read_type::uring));
    CHECK(file.is_open());

    char buf[100];
    std::error_code ec;
    size_t size;
    std::tie(ec, size) =
        async_simple::coro::syncAwait(file.async_read_at(0, buf, 10));
    CHECK(ec == std::make_error_code(std::errc::bad_file_descriptor));
    CHECK(size == 0);

    ec = async_simple::coro::syncAwait(file.async_write_at(0, buf, 10));
    CHECK(ec == std::make_error_code(std::errc::bad_file_descriptor));
  }
#else
  {
    coro_io::coro_file file{};
    async_simple::coro::syncAwait(file.async_open(
        filename.data(), coro_io::flags::read_only, coro_io::read_type::fread));
    CHECK(file.is_open());

    char buf[100];
    std::error_code ec;
    size_t size;
    std::tie(ec, size) =
        async_simple::coro::syncAwait(file.async_pread(0, buf, 10));
    CHECK(ec == std::make_error_code(std::errc::bad_file_descriptor));
    CHECK(size == 0);

    auto write_ec =
        async_simple::coro::syncAwait(file.async_pwrite(0, buf, 10));
    CHECK(write_ec == std::make_error_code(std::errc::bad_file_descriptor));
  }
#endif
}

TEST_CASE("coro_file pread and pwrite basic test") {
  std::string filename = "test.tmp";
  create_files({filename}, 190);
  {
    coro_io::coro_file file{};
    async_simple::coro::syncAwait(file.async_open(
        filename.data(), coro_io::flags::read_only, coro_io::read_type::pread));
    CHECK(file.is_open());

    char buf[100];
    auto pair = async_simple::coro::syncAwait(file.async_pread(0, buf, 10));
    CHECK(std::string_view(buf, pair.second) == "AAAAAAAAAA");
    CHECK(!file.eof());

    pair = async_simple::coro::syncAwait(file.async_pread(10, buf, 100));
    CHECK(!file.eof());
    CHECK(pair.second == 100);

    pair = async_simple::coro::syncAwait(file.async_pread(110, buf, 100));
    CHECK(!file.eof());
    CHECK(pair.second == 80);

    // only read size equal 0 is eof.
    pair = async_simple::coro::syncAwait(file.async_pread(200, buf, 100));
    CHECK(file.eof());
    CHECK(pair.second == 0);
  }

#if defined(YLT_ENABLE_FILE_IO_URING)
  {
    coro_io::coro_file file{};
    async_simple::coro::syncAwait(
        file.async_open(filename.data(), coro_io::flags::read_only,
                        coro_io::read_type::uring_random));
    CHECK(file.is_open());

    char buf[100];
    auto pair = async_simple::coro::syncAwait(file.async_read_at(0, buf, 10));
    CHECK(std::string_view(buf, pair.second) == "AAAAAAAAAA");
    CHECK(!file.eof());

    pair = async_simple::coro::syncAwait(file.async_read_at(10, buf, 100));
    CHECK(!file.eof());
    CHECK(pair.second == 100);

    pair = async_simple::coro::syncAwait(file.async_read_at(110, buf, 100));
    CHECK(pair.second == 80);

    // only read size equal 0 is eof.
    pair = async_simple::coro::syncAwait(file.async_read_at(200, buf, 100));
    CHECK(file.eof());
    CHECK(pair.second == 0);
  }

  {
    coro_io::coro_file file{};
    async_simple::coro::syncAwait(
        file.async_open(filename.data(), coro_io::flags::read_write,
                        coro_io::read_type::uring_random));
    CHECK(file.is_open());

    std::string buf = "cccccccccc";
    auto ec = async_simple::coro::syncAwait(
        file.async_write_at(0, buf.data(), buf.size()));
    CHECK(!ec);

    std::string buf1 = "dddddddddd";
    ec = async_simple::coro::syncAwait(
        file.async_write_at(10, buf1.data(), buf1.size()));
    CHECK(!ec);

    char buf2[100];
    auto pair = async_simple::coro::syncAwait(file.async_read_at(0, buf2, 10));
    CHECK(!file.eof());
    CHECK(std::string_view(buf2, pair.second) == "cccccccccc");

    pair = async_simple::coro::syncAwait(file.async_read_at(10, buf2, 10));
    CHECK(!file.eof());
    CHECK(std::string_view(buf2, pair.second) == "dddddddddd");
  }
#endif

  {
    coro_io::coro_file file{};
    async_simple::coro::syncAwait(file.async_open(filename.data(),
                                                  coro_io::flags::read_write,
                                                  coro_io::read_type::pread));
    CHECK(file.is_open());

    std::string buf = "cccccccccc";
    auto ec = async_simple::coro::syncAwait(
        file.async_pwrite(0, buf.data(), buf.size()));
    CHECK(!ec);

    std::string buf1 = "dddddddddd";
    ec = async_simple::coro::syncAwait(
        file.async_pwrite(10, buf1.data(), buf1.size()));
    CHECK(!ec);

    char buf2[100];
    auto pair = async_simple::coro::syncAwait(file.async_pread(0, buf2, 10));
    CHECK(!file.eof());
    CHECK(std::string_view(buf2, pair.second) == "cccccccccc");

    pair = async_simple::coro::syncAwait(file.async_pread(10, buf2, 10));
    CHECK(!file.eof());
    CHECK(std::string_view(buf2, pair.second) == "dddddddddd");
  }
}

async_simple::coro::Lazy<void> test_basic_read(std::string filename) {
  coro_io::coro_file file{};
  co_await file.async_open(filename.data(), coro_io::flags::read_only);
  std::string str;
  str.resize(200);

  {
    auto [ec, size] = co_await file.async_read(str.data(), 10);
    std::cout << size << ", " << file.eof() << "\n";
  }
  {
    bool ok = file.seek(10, SEEK_CUR);
    std::cout << ok << "\n";
  }
  {
    auto [ec, size] = co_await file.async_read(str.data(), str.size());
    std::cout << size << ", " << file.eof() << "\n";
  }
}

async_simple::coro::Lazy<void> test_basic_write(std::string filename) {
  coro_io::coro_file file{};
  co_await file.async_open(filename.data(), coro_io::flags::read_write);
  std::string str = "hello";

  {
    auto ec = co_await file.async_write(str.data(), str.size());
    std::string result;
    result.resize(10);
    file.seek(0, SEEK_SET);
    auto [rd_ec, size] = co_await file.async_read(result.data(), 5);
    std::string_view s(result.data(), size);
    CHECK(s == "hello");
  }
  {
    bool ok = file.seek(10, SEEK_SET);
    auto ec = co_await file.async_write(str.data(), str.size());
    file.seek(10, SEEK_SET);
    std::string result;
    result.resize(10);
    auto [rd_ec, size] = co_await file.async_read(result.data(), 5);
    std::string_view s(result.data(), size);
    CHECK(s == "hello");

    std::cout << ec << "\n";
  }
}

TEST_CASE("multithread for balance") {
  size_t total = 100;
  std::vector<std::string> filenames;
  for (size_t i = 0; i < total; ++i) {
    filenames.push_back("temp" + std::to_string(i + 1));
  }

  std::vector<std::string> write_str_vec;
  char ch = 'a';
  for (int i = 0; i < 26; ++i) {
    std::string str(100, ch + i);
    write_str_vec.push_back(std::move(str));
  }

  std::vector<async_simple::coro::Lazy<void> > write_vec;
  auto write_file_func =
      [&write_str_vec](std::string filename,
                       int index) mutable -> async_simple::coro::Lazy<void> {
    coro_io::coro_file file(coro_io::get_global_block_executor<
                            coro_io::multithread_context_pool>());
    async_simple::coro::syncAwait(
        file.async_open(filename, coro_io::flags::create_write));
    CHECK(file.is_open());

    size_t id = index % write_str_vec.size();
    auto& str = write_str_vec[id];
    auto ec = co_await file.async_write(str.data(), str.size());
    CHECK(!ec);
    co_return;
  };

  for (size_t i = 0; i < total; ++i) {
    write_vec.push_back(write_file_func(filenames[i], i));
  }

  auto wait_func =
      [write_vec =
           std::move(write_vec)]() mutable -> async_simple::coro::Lazy<void> {
    co_await async_simple::coro::collectAll(std::move(write_vec));
  };

  async_simple::coro::syncAwait(wait_func());

  // read and compare
  std::vector<async_simple::coro::Lazy<void> > read_vec;

  auto read_file_func =
      [&write_str_vec](std::string filename,
                       int index) mutable -> async_simple::coro::Lazy<void> {
    coro_io::coro_file file(coro_io::get_global_block_executor<
                            coro_io::multithread_context_pool>());
    async_simple::coro::syncAwait(
        file.async_open(filename, coro_io::flags::read_only));
    CHECK(file.is_open());

    size_t id = index % write_str_vec.size();
    auto& str = write_str_vec[id];
    std::string buf;
    buf.resize(write_str_vec.back().size());

    std::error_code ec;
    size_t read_size;
    std::tie(ec, read_size) = co_await file.async_read(buf.data(), buf.size());
    CHECK(!ec);
    CHECK(str == buf);
    co_return;
  };

  for (size_t i = 0; i < total; ++i) {
    read_vec.push_back(read_file_func(filenames[i], i));
  }

  auto wait_read_func =
      [read_vec =
           std::move(read_vec)]() mutable -> async_simple::coro::Lazy<void> {
    co_await async_simple::coro::collectAll(std::move(read_vec));
  };

  async_simple::coro::syncAwait(wait_read_func());

  for (auto& filename : filenames) {
    fs::remove(fs::path(filename));
  }
}

TEST_CASE("read write 100 small files") {
  size_t total = 100;
  std::vector<std::string> filenames;
  for (size_t i = 0; i < total; ++i) {
    filenames.push_back("temp" + std::to_string(i + 1));
  }

  coro_io::io_context_pool pool(std::thread::hardware_concurrency());
  std::thread thd([&pool] {
    pool.run();
  });

  std::vector<std::string> write_str_vec;
  char ch = 'a';
  for (int i = 0; i < 26; ++i) {
    std::string str(100, ch + i);
    write_str_vec.push_back(std::move(str));
  }

  std::vector<async_simple::coro::Lazy<void> > write_vec;

  auto write_file_func =
      [&pool, &write_str_vec](
          std::string filename,
          int index) mutable -> async_simple::coro::Lazy<void> {
    coro_io::coro_file file(pool.get_executor());
    async_simple::coro::syncAwait(
        file.async_open(filename, coro_io::flags::create_write));
    CHECK(file.is_open());

    size_t id = index % write_str_vec.size();
    auto& str = write_str_vec[id];
    auto ec = co_await file.async_write(str.data(), str.size());
    CHECK(!ec);
    co_return;
  };

  for (size_t i = 0; i < total; ++i) {
    write_vec.push_back(write_file_func(filenames[i], i));
  }

  auto wait_func =
      [write_vec =
           std::move(write_vec)]() mutable -> async_simple::coro::Lazy<void> {
    co_await async_simple::coro::collectAll(std::move(write_vec));
  };

  async_simple::coro::syncAwait(wait_func());

  // read and compare
  std::vector<async_simple::coro::Lazy<void> > read_vec;

  auto read_file_func =
      [&pool, &write_str_vec](
          std::string filename,
          int index) mutable -> async_simple::coro::Lazy<void> {
    coro_io::coro_file file(pool.get_executor());
    async_simple::coro::syncAwait(
        file.async_open(filename, coro_io::flags::read_only));
    CHECK(file.is_open());

    size_t id = index % write_str_vec.size();
    auto& str = write_str_vec[id];
    std::string buf;
    buf.resize(write_str_vec.back().size());

    std::error_code ec;
    size_t read_size;
    std::tie(ec, read_size) = co_await file.async_read(buf.data(), buf.size());
    CHECK(!ec);
    CHECK(str == buf);
    co_return;
  };

  for (size_t i = 0; i < total; ++i) {
    read_vec.push_back(read_file_func(filenames[i], i));
  }

  auto wait_read_func =
      [read_vec =
           std::move(read_vec)]() mutable -> async_simple::coro::Lazy<void> {
    co_await async_simple::coro::collectAll(std::move(read_vec));
  };

  async_simple::coro::syncAwait(wait_read_func());

  pool.stop();
  thd.join();

  for (auto& filename : filenames) {
    fs::remove(fs::path(filename));
  }
}

TEST_CASE("small_file_read_test") {
  std::string filename = "small_file_read_test.txt";
  std::string fill_with = "small_file_read_test";
  auto block_vec = create_filled_vec(fill_with);
  create_file(filename, 1 * KB, block_vec);
  asio::io_context ioc;
  auto work = std::make_unique<asio::io_context::work>(ioc);
  std::thread thd([&ioc] {
    ioc.run();
  });

  coro_io::coro_file file(ioc.get_executor());
  async_simple::coro::syncAwait(
      file.async_open(filename, coro_io::flags::read_only));
  CHECK(file.is_open());

  char buf[block_size]{};
  std::error_code ec;
  size_t read_size;
  while (!file.eof()) {
    std::tie(ec, read_size) =
        async_simple::coro::syncAwait(file.async_read(buf, block_size));
    if (ec) {
      std::cout << ec.message() << "\n";
      break;
    }
    std::cout << read_size << std::endl;
    CHECK(std::string_view(block_vec.data(), read_size) ==
          std::string_view(buf, read_size));
  }
  work.reset();
  thd.join();
  file.close();
  fs::remove(fs::path(filename));
}
TEST_CASE("large_file_read_test") {
  std::string filename = "large_file_read_test.txt";
  std::string fill_with = "large_file_read_test";
  size_t file_size = 100 * MB;
  auto block_vec = create_filled_vec(fill_with);
  create_file(filename, file_size, block_vec);
  CHECK(fs::file_size(filename) == file_size);
  asio::io_context ioc;
  auto work = std::make_unique<asio::io_context::work>(ioc);
  std::thread thd([&ioc] {
    ioc.run();
  });

  coro_io::coro_file file(ioc.get_executor());
  async_simple::coro::syncAwait(
      file.async_open(filename, coro_io::flags::read_only));
  CHECK(file.is_open());

  char buf[block_size]{};
  size_t total_size = 0;
  std::error_code ec;
  size_t read_size;
  while (!file.eof()) {
    std::tie(ec, read_size) =
        async_simple::coro::syncAwait(file.async_read(buf, block_size));
    if (ec) {
      std::cout << ec.message() << "\n";
      break;
    }
    total_size += read_size;
    CHECK(std::string_view(block_vec.data(), read_size) ==
          std::string_view(buf, read_size));
  }
  CHECK(total_size == file_size);
  work.reset();
  thd.join();
  file.close();
  fs::remove(fs::path(filename));
}
TEST_CASE("empty_file_read_test") {
  std::string filename = "empty_file_read_test.txt";
  std::string fill_with = "";
  auto block_vec = create_filled_vec(fill_with);
  create_file(filename, 0, block_vec);
  asio::io_context ioc;
  auto work = std::make_unique<asio::io_context::work>(ioc);
  std::thread thd([&ioc] {
    ioc.run();
  });

  coro_io::coro_file file(ioc.get_executor());
  async_simple::coro::syncAwait(
      file.async_open(filename, coro_io::flags::read_only));
  CHECK(file.is_open());

  char buf[block_size]{};
  std::error_code ec;
  size_t read_size;
  std::tie(ec, read_size) =
      async_simple::coro::syncAwait(file.async_read(buf, block_size));
  if (ec) {
    std::cout << ec.message() << "\n";
  }
  auto read_content = std::string_view(buf, read_size);
  CHECK(read_size == 0);
  CHECK(read_content.empty());
  work.reset();
  thd.join();
  file.close();
  fs::remove(fs::path(filename));
}
TEST_CASE("small_file_read_with_pool_test") {
  std::string filename = "small_file_read_with_pool_test.txt";
  std::string fill_with = "small_file_read_with_pool_test";
  size_t file_size = 1 * KB;
  auto block_vec = create_filled_vec(fill_with);
  create_file(filename, file_size, block_vec);
  CHECK(fs::file_size(filename) == file_size);
  coro_io::io_context_pool pool(std::thread::hardware_concurrency());
  std::thread thd([&pool] {
    pool.run();
  });

  coro_io::coro_file file(pool.get_executor());
  async_simple::coro::syncAwait(
      file.async_open(filename, coro_io::flags::read_only));
  CHECK(file.is_open());

  char buf[block_size]{};
  std::error_code ec;
  size_t read_size;
  while (!file.eof()) {
    std::tie(ec, read_size) =
        async_simple::coro::syncAwait(file.async_read(buf, block_size));
    if (ec) {
      std::cout << ec.message() << "\n";
      break;
    }
    std::cout << read_size << std::endl;
    CHECK(std::string_view(block_vec.data(), read_size) ==
          std::string_view(buf, read_size));
  }
  pool.stop();
  thd.join();
  file.close();
  fs::remove(fs::path(filename));
}
TEST_CASE("large_file_read_with_pool_test") {
  std::string filename = "large_file_read_with_pool_test.txt";
  std::string fill_with = "large_file_read_with_pool_test";
  size_t file_size = 100 * MB;
  auto block_vec = create_filled_vec(fill_with);
  create_file(filename, file_size, block_vec);
  CHECK(fs::file_size(filename) == file_size);
  coro_io::io_context_pool pool(std::thread::hardware_concurrency());
  std::thread thd([&pool] {
    pool.run();
  });

  coro_io::coro_file file(pool.get_executor());
  async_simple::coro::syncAwait(
      file.async_open(filename, coro_io::flags::read_only));
  CHECK(file.is_open());

  char buf[block_size]{};
  size_t total_size = 0;
  std::error_code ec;
  size_t read_size;
  while (!file.eof()) {
    std::tie(ec, read_size) =
        async_simple::coro::syncAwait(file.async_read(buf, block_size));
    if (ec) {
      std::cout << ec.message() << "\n";
      break;
    }
    total_size += read_size;
    CHECK(std::string_view(block_vec.data(), read_size) ==
          std::string_view(buf, read_size));
  }
  CHECK(total_size == file_size);
  pool.stop();
  thd.join();
  file.close();
  fs::remove(fs::path(filename));
}
TEST_CASE("small_file_write_test") {
  std::string filename = "small_file_write_test.txt";
  asio::io_context ioc;
  auto work = std::make_unique<asio::io_context::work>(ioc);
  std::thread thd([&ioc] {
    ioc.run();
  });

  coro_io::coro_file file(ioc.get_executor());
  async_simple::coro::syncAwait(
      file.async_open(filename, coro_io::flags::create_write));
  CHECK(file.is_open());

  char buf[512]{};

  std::string file_content_0 = "small_file_write_test_0";

  auto ec = async_simple::coro::syncAwait(
      file.async_write(file_content_0.data(), file_content_0.size()));
  if (ec) {
    std::cout << ec.message() << "\n";
  }

  file.flush();

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
  file.flush();
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
  file.close();
  fs::remove(fs::path(filename));
}
TEST_CASE("large_file_write_test") {
  std::string filename = "large_file_write_test.txt";
  size_t file_size = 100 * MB;
  asio::io_context ioc;
  auto work = std::make_unique<asio::io_context::work>(ioc);
  std::thread thd([&ioc] {
    ioc.run();
  });

  coro_io::coro_file file(ioc.get_executor());
  async_simple::coro::syncAwait(
      file.async_open(filename, coro_io::flags::create_write));
  CHECK(file.is_open());

  auto block_vec = create_filled_vec("large_file_write_test");
  int cnt = file_size / block_size;
  int remain = file_size % block_size;
  while (cnt--) {
    auto ec = async_simple::coro::syncAwait(
        file.async_write(block_vec.data(), block_size));
    if (ec) {
      std::cout << ec.message() << "\n";
      break;
    }
  }
  if (remain > 0) {
    auto ec = async_simple::coro::syncAwait(
        file.async_write(block_vec.data(), remain));
    if (ec) {
      std::cout << ec.message() << "\n";
    }
  }
  file.flush();
  CHECK(fs::file_size(filename) == file_size);
  std::ifstream is(filename, std::ios::binary);
  if (!is.is_open()) {
    std::cout << "Failed to open file: " << filename << "\n";
    return;
  }
  is.seekg(0, std::ios::end);
  auto size = is.tellg();
  is.seekg(0, std::ios::beg);
  CHECK(size == file_size);

  std::vector<char> read_content(block_size);
  while (!is.eof()) {
    is.read(read_content.data(), block_size);
    CHECK(std::string_view(read_content.data(), is.gcount()) ==
          std::string_view(block_vec.data(), is.gcount()));
  }
  is.close();
  work.reset();
  thd.join();
  file.close();
  fs::remove(fs::path(filename));
}
TEST_CASE("empty_file_write_test") {
  std::string filename = "empty_file_write_test.txt";
  asio::io_context ioc;
  auto work = std::make_unique<asio::io_context::work>(ioc);
  std::thread thd([&ioc] {
    ioc.run();
  });

  coro_io::coro_file file(ioc.get_executor());
  async_simple::coro::syncAwait(
      file.async_open(filename, coro_io::flags::create_write));
  CHECK(file.is_open());

  char buf[512]{};

  std::string file_content_0 = "small_file_write_test_0";

  auto ec =
      async_simple::coro::syncAwait(file.async_write(file_content_0.data(), 0));
  if (ec) {
    std::cout << ec.message() << "\n";
  }
  file.flush();
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
  file.close();
  fs::remove(fs::path(filename));
}
TEST_CASE("small_file_write_with_pool_test") {
  std::string filename = "small_file_write_with_pool_test.txt";
  coro_io::io_context_pool pool(std::thread::hardware_concurrency());
  std::thread thd([&pool] {
    pool.run();
  });

  coro_io::coro_file file(pool.get_executor());
  async_simple::coro::syncAwait(
      file.async_open(filename, coro_io::flags::create_write));
  CHECK(file.is_open());

  char buf[512]{};

  std::string file_content_0 = "small_file_write_with_pool_test_0";

  auto ec = async_simple::coro::syncAwait(
      file.async_write(file_content_0.data(), file_content_0.size()));
  if (ec) {
    std::cout << ec.message() << "\n";
  }
  file.flush();

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

  std::string file_content_1 = "small_file_write_with_pool_test_1";

  ec = async_simple::coro::syncAwait(
      file.async_write(file_content_1.data(), file_content_1.size()));
  if (ec) {
    std::cout << ec.message() << "\n";
  }
  file.flush();
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

  pool.stop();
  thd.join();
  file.close();
  fs::remove(fs::path(filename));
}
TEST_CASE("large_file_write_with_pool_test") {
  std::string filename = "large_file_write_with_pool_test.txt";
  size_t file_size = 100 * MB;
  coro_io::io_context_pool pool(std::thread::hardware_concurrency());
  std::thread thd([&pool] {
    pool.run();
  });

  coro_io::coro_file file(pool.get_executor());
  async_simple::coro::syncAwait(
      file.async_open(filename, coro_io::flags::create_write));
  CHECK(file.is_open());

  auto block_vec = create_filled_vec("large_file_write_with_pool_test");
  int cnt = file_size / block_size;
  int remain = file_size % block_size;
  while (cnt--) {
    auto ec = async_simple::coro::syncAwait(
        file.async_write(block_vec.data(), block_size));
    if (ec) {
      std::cout << ec.message() << "\n";
      break;
    }
  }
  if (remain > 0) {
    auto ec = async_simple::coro::syncAwait(
        file.async_write(block_vec.data(), remain));
    if (ec) {
      std::cout << ec.message() << "\n";
    }
  }
  file.flush();
  size_t sz = fs::file_size(filename);
  CHECK(sz == file_size);
  std::ifstream is(filename, std::ios::binary);
  if (!is.is_open()) {
    std::cout << "Failed to open file: " << filename << "\n";
    return;
  }
  is.seekg(0, std::ios::end);
  auto size = is.tellg();
  is.seekg(0, std::ios::beg);
  CHECK(size == file_size);

  std::vector<char> read_content(block_size);
  while (!is.eof()) {
    is.read(read_content.data(), block_size);
    CHECK(std::string_view(read_content.data(), is.gcount()) ==
          std::string_view(block_vec.data(), is.gcount()));
  }
  is.close();
  pool.stop();
  thd.join();
  file.close();
  fs::remove(fs::path(filename));
}
