#include <async_simple/coro/Collect.h>
#include <async_simple/coro/SyncAwait.h>
#include <doctest.h>
#include <fcntl.h>

#include <array>
#include <asio/io_context.hpp>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>
#include <ylt/coro_io/coro_file.hpp>
#include <ylt/coro_io/shared_file_handle.hpp>

#if defined(ASIO_WINDOWS)
#include <io.h>
#include <windows.h>
#else
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace {

namespace fs = std::filesystem;

#if defined(ASIO_WINDOWS)
constexpr int read_only_flag = _O_RDONLY;
#else
constexpr int read_only_flag = O_RDONLY;
#endif

class test_file {
 public:
  test_file(std::string path, std::string_view content)
      : path_(std::move(path)) {
    std::ofstream output(path_, std::ios::binary);
    output.write(content.data(), static_cast<std::streamsize>(content.size()));
  }

  ~test_file() {
    std::error_code ec;
    fs::remove(path_, ec);
  }

  const std::string &path() const { return path_; }

 private:
  std::string path_;
};

int open_read_only(const std::string &path) {
#if defined(ASIO_WINDOWS)
  return ::_open(path.c_str(), read_only_flag);
#else
  return ::open(path.c_str(), read_only_flag);
#endif
}

void close_fd(int fd) {
#if defined(ASIO_WINDOWS)
  ::_close(fd);
#else
  ::close(fd);
#endif
}

class file_descriptor_probe {
 public:
  explicit file_descriptor_probe(int fd)
#if defined(ASIO_WINDOWS)
      : handle_(::_get_osfhandle(fd)) {
  }

  bool is_open() const {
    DWORD flags = 0;
    return ::GetHandleInformation(reinterpret_cast<HANDLE>(handle_), &flags) !=
           0;
  }

 private:
  std::intptr_t handle_;
#else
      : fd_(fd) {
  }

  bool is_open() const { return ::fcntl(fd_, F_GETFD) != -1; }

 private:
  int fd_;
#endif
};

#if defined(ASIO_WINDOWS)
using windows_native_async_file =
    coro_io::basic_random_coro_file<coro_io::execution_type::native_async>;
using windows_thread_pool_file =
    coro_io::basic_random_coro_file<coro_io::execution_type::thread_pool>;
static_assert(!std::is_constructible_v<windows_native_async_file,
                                       coro_io::shared_file_handle>);
static_assert(std::is_constructible_v<windows_thread_pool_file,
                                      coro_io::shared_file_handle>);
#endif

class io_context_runner {
 public:
  explicit io_context_runner(asio::io_context &io_context)
      : io_context_(io_context),
        work_(std::make_unique<asio::io_context::work>(io_context_)),
        thread_([this] {
          io_context_.run();
        }) {}

  ~io_context_runner() {
    work_.reset();
    io_context_.stop();
    thread_.join();
  }

 private:
  asio::io_context &io_context_;
  std::unique_ptr<asio::io_context::work> work_;
  std::thread thread_;
};

#if defined(__linux__)
size_t count_file_descriptors_for_file(int source_fd) {
  struct stat source_stat {};
  if (::fstat(source_fd, &source_stat) != 0) {
    return 0;
  }

  size_t count = 0;
  for (const auto &entry : fs::directory_iterator("/proc/self/fd")) {
    struct stat candidate_stat {};
    if (::stat(entry.path().c_str(), &candidate_stat) == 0 &&
        candidate_stat.st_dev == source_stat.st_dev &&
        candidate_stat.st_ino == source_stat.st_ino) {
      ++count;
    }
  }
  return count;
}
#endif

template <coro_io::execution_type execute_type>
void check_inflight_operation_retains_handle() {
  const std::string content = "shared file handle in-flight read";
  test_file source("shared_file_handle_inflight.tmp", content);
  test_file pressure("shared_file_handle_pressure.tmp", "other file");

  auto open_result =
      coro_io::shared_file_handle::open(source.path(), read_only_flag);
  REQUIRE_FALSE(open_result.first);
  auto handle = std::move(open_result.second);
  REQUIRE(handle.valid());
  const int fd = handle.native_handle();
  const file_descriptor_probe fd_probe(fd);

  asio::io_context io_context;
  auto file = std::make_unique<coro_io::basic_random_coro_file<execute_type>>(
      handle, io_context.get_executor(), source.path());
  REQUIRE(file->is_open());

  std::array<char, 64> buffer{};
  std::pair<std::error_code, size_t> result;
  bool completed = false;
  file->async_read_at(0, buffer.data(), content.size())
      .start([&](auto &&value) {
        result = value.value();
        completed = true;
      });

  file.reset();
  handle = {};
  REQUIRE(fd_probe.is_open());
  CHECK_FALSE(completed);

  std::vector<int> pressure_fds;
  for (size_t index = 0; index < 64; ++index) {
    int pressure_fd = open_read_only(pressure.path());
    REQUIRE(pressure_fd >= 0);
    CHECK(pressure_fd != fd);
    pressure_fds.push_back(pressure_fd);
  }

  io_context.run();
  REQUIRE(completed);
  CHECK_FALSE(result.first);
  CHECK(result.second == content.size());
  CHECK(std::string_view(buffer.data(), content.size()) == content);
  CHECK_FALSE(fd_probe.is_open());

  for (int pressure_fd : pressure_fds) {
    close_fd(pressure_fd);
  }
}

TEST_CASE("shared_file_handle lifetime and factories") {
  test_file source("shared_file_handle_factories.tmp", "shared handle");

  SUBCASE("open and weak handle") {
    auto open_result =
        coro_io::shared_file_handle::open(source.path(), read_only_flag);
    REQUIRE_FALSE(open_result.first);
    auto handle = std::move(open_result.second);
    REQUIRE(handle.valid());

    const int fd = handle.native_handle();
    const file_descriptor_probe fd_probe(fd);
    auto weak = handle.weak();
    {
      auto copy = handle;
      CHECK(copy.native_handle() == fd);
      CHECK(weak.lock().native_handle() == fd);
    }

    CHECK(fd_probe.is_open());
    handle = {};
    CHECK(weak.expired());
    CHECK_FALSE(fd_probe.is_open());
  }

  SUBCASE("adopt") {
    int fd = open_read_only(source.path());
    REQUIRE(fd >= 0);
    const file_descriptor_probe fd_probe(fd);
    {
      auto handle = coro_io::shared_file_handle::adopt(fd);
      REQUIRE(handle.valid());
      CHECK(handle.native_handle() == fd);
    }
    CHECK_FALSE(fd_probe.is_open());
  }

  SUBCASE("duplicate") {
    int fd = open_read_only(source.path());
    REQUIRE(fd >= 0);
    auto duplicate_result = coro_io::shared_file_handle::duplicate(fd);
    REQUIRE_FALSE(duplicate_result.first);
    auto handle = std::move(duplicate_result.second);
    REQUIRE(handle.valid());
    const int duplicated_fd = handle.native_handle();
    const file_descriptor_probe duplicated_fd_probe(duplicated_fd);
    CHECK(duplicated_fd != fd);

    close_fd(fd);
    CHECK(duplicated_fd_probe.is_open());
    handle = {};
    CHECK_FALSE(duplicated_fd_probe.is_open());
  }

  SUBCASE("open failure") {
    auto open_result = coro_io::shared_file_handle::open(
        "shared_file_handle_missing/file", read_only_flag);
    CHECK(open_result.first);
    CHECK_FALSE(open_result.second.valid());
  }
}

TEST_CASE("thread pool wrappers share one file descriptor") {
  const std::string content = "0123456789abcdefghijklmnopqrstuvwxyz";
  test_file source("shared_file_handle_thread_pool.tmp", content);
  auto open_result =
      coro_io::shared_file_handle::open(source.path(), read_only_flag);
  REQUIRE_FALSE(open_result.first);
  auto handle = std::move(open_result.second);
  const int fd = handle.native_handle();
  const file_descriptor_probe fd_probe(fd);

  asio::io_context io_context;
  io_context_runner runner(io_context);

  coro_io::basic_random_coro_file<coro_io::execution_type::thread_pool> first(
      handle, io_context.get_executor());
  coro_io::basic_random_coro_file<coro_io::execution_type::thread_pool> second(
      handle, io_context.get_executor(), source.path());
  CHECK(first.file_path().empty());
  CHECK(first.file_size() == content.size());
  std::error_code file_size_error =
      std::make_error_code(std::errc::invalid_argument);
  CHECK(first.file_size(file_size_error) == content.size());
  CHECK_FALSE(file_size_error);
#if defined(__linux__)
  CHECK(count_file_descriptors_for_file(fd) == 1);
#endif
  handle = {};

  std::array<char, 6> first_buffer{};
  auto first_result = async_simple::coro::syncAwait(
      first.async_read_at(0, first_buffer.data(), first_buffer.size()));
  CHECK_FALSE(first_result.first);
  CHECK(std::string_view(first_buffer.data(), first_buffer.size()) == "012345");

  first.close();
  CHECK(fd_probe.is_open());

  std::array<char, 6> second_buffer{};
  auto second_result = async_simple::coro::syncAwait(
      second.async_read_at(6, second_buffer.data(), second_buffer.size()));
  CHECK_FALSE(second_result.first);
  CHECK(std::string_view(second_buffer.data(), second_buffer.size()) ==
        "6789ab");

  second.close();
  CHECK_FALSE(fd_probe.is_open());
}

TEST_CASE("closed random_coro_file rejects new operations") {
  test_file source("shared_file_handle_closed.tmp", "closed");
  auto open_result =
      coro_io::shared_file_handle::open(source.path(), read_only_flag);
  REQUIRE_FALSE(open_result.first);
  auto handle = std::move(open_result.second);

  coro_io::basic_random_coro_file<coro_io::execution_type::thread_pool> file(
      handle, coro_io::get_global_block_executor(), source.path());
  file.close();

  std::error_code file_size_error =
      std::make_error_code(std::errc::invalid_argument);
  CHECK(file.file_size(file_size_error) == 6);
  CHECK_FALSE(file_size_error);
  CHECK(file.file_size() == 6);

  char buffer{};
  auto read_result =
      async_simple::coro::syncAwait(file.async_read_at(0, &buffer, 1));
  CHECK(read_result.first == std::errc::bad_file_descriptor);
  CHECK(read_result.second == 0);
}

TEST_CASE("random_coro_file reports native handle size errors") {
  auto handle = coro_io::shared_file_handle::adopt(
      std::numeric_limits<
          coro_io::shared_file_handle::native_handle_type>::max());
  REQUIRE(handle.valid());

  coro_io::basic_random_coro_file<coro_io::execution_type::thread_pool> file(
      handle);
  std::error_code ec;
  CHECK(file.file_size(ec) == static_cast<size_t>(-1));
  CHECK(ec == std::errc::bad_file_descriptor);
  CHECK_THROWS_AS(file.file_size(), std::system_error);
}

TEST_CASE("thread pool operation retains handle after wrapper destruction") {
  check_inflight_operation_retains_handle<
      coro_io::execution_type::thread_pool>();
}

#if defined(ASIO_HAS_FILE) && !defined(ASIO_WINDOWS)
TEST_CASE("native async wrappers share one file descriptor") {
  constexpr size_t wrapper_count = 8;
  constexpr size_t read_size = 32;
  std::string content;
  for (size_t index = 0; index < wrapper_count; ++index) {
    content.append(read_size, static_cast<char>('A' + index));
  }
  test_file source("shared_file_handle_native_async.tmp", content);

  auto open_result =
      coro_io::shared_file_handle::open(source.path(), read_only_flag);
  REQUIRE_FALSE(open_result.first);
  auto handle = std::move(open_result.second);
  const int fd = handle.native_handle();
  const file_descriptor_probe fd_probe(fd);

  asio::io_context io_context;
  io_context_runner runner(io_context);

  using random_file =
      coro_io::basic_random_coro_file<coro_io::execution_type::native_async>;
  std::vector<std::unique_ptr<random_file>> files;
  std::vector<std::string> buffers(wrapper_count, std::string(read_size, '\0'));
  std::vector<async_simple::coro::Lazy<std::pair<std::error_code, size_t>>>
      operations;
  for (size_t index = 0; index < wrapper_count; ++index) {
    files.push_back(std::make_unique<random_file>(
        handle, io_context.get_executor(), source.path()));
    REQUIRE(files.back()->is_open());
    operations.push_back(files.back()->async_read_at(
        index * read_size, buffers[index].data(), read_size));
  }
#if defined(__linux__)
  CHECK(count_file_descriptors_for_file(fd) == 1);
#endif

  auto results = async_simple::coro::syncAwait(
      async_simple::coro::collectAll(std::move(operations)));
  REQUIRE(results.size() == wrapper_count);
  for (size_t index = 0; index < wrapper_count; ++index) {
    auto read_result = results[index].value();
    CHECK_FALSE(read_result.first);
    CHECK(read_result.second == read_size);
    CHECK(buffers[index] ==
          std::string(read_size, static_cast<char>('A' + index)));
  }

  files.front()->close();
  CHECK(fd_probe.is_open());
  files.clear();
  CHECK(fd_probe.is_open());
  handle = {};
  CHECK_FALSE(fd_probe.is_open());
}

TEST_CASE("native async operation retains handle after wrapper destruction") {
  check_inflight_operation_retains_handle<
      coro_io::execution_type::native_async>();
}
#endif

}  // namespace
