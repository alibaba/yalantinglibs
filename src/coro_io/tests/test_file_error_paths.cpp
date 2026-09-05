#include <async_simple/coro/SyncAwait.h>
#include <doctest.h>
#include <fcntl.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <new>
#include <string>
#include <string_view>
#include <system_error>
#include <ylt/coro_io/coro_file.hpp>
#include <ylt/coro_io/shared_file_handle.hpp>

#if defined(ASIO_WINDOWS)
#include <io.h>
#else
#include <unistd.h>
#endif

namespace allocation_failure {

thread_local bool fail_next = false;

bool consume_failure() noexcept {
  if (!fail_next) {
    return false;
  }
  fail_next = false;
  return true;
}

class fail_next_allocation {
 public:
  fail_next_allocation() noexcept { fail_next = true; }
  ~fail_next_allocation() { fail_next = false; }

  fail_next_allocation(const fail_next_allocation &) = delete;
  fail_next_allocation &operator=(const fail_next_allocation &) = delete;
};

}  // namespace allocation_failure

#if SIZE_MAX == UINT64_MAX
extern "C" void *__real__Znwm(std::size_t size);
extern "C" void *__real__Znam(std::size_t size);

extern "C" void *__wrap__Znwm(std::size_t size) {
  if (allocation_failure::consume_failure()) {
    throw std::bad_alloc{};
  }
  return __real__Znwm(size);
}

extern "C" void *__wrap__Znam(std::size_t size) {
  if (allocation_failure::consume_failure()) {
    throw std::bad_alloc{};
  }
  return __real__Znam(size);
}
#elif SIZE_MAX == UINT32_MAX
extern "C" void *__real__Znwj(std::size_t size);
extern "C" void *__real__Znaj(std::size_t size);

extern "C" void *__wrap__Znwj(std::size_t size) {
  if (allocation_failure::consume_failure()) {
    throw std::bad_alloc{};
  }
  return __real__Znwj(size);
}

extern "C" void *__wrap__Znaj(std::size_t size) {
  if (allocation_failure::consume_failure()) {
    throw std::bad_alloc{};
  }
  return __real__Znaj(size);
}
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

bool fd_is_open(int fd) {
#if defined(ASIO_WINDOWS)
  return ::_get_osfhandle(fd) != -1;
#else
  return ::fcntl(fd, F_GETFD) != -1;
#endif
}

std::ios::openmode runtime_open_mode(std::ios::openmode mode) {
  volatile auto value = mode;
  return value;
}

TEST_CASE("file open modes cover every flag mapping") {
  CHECK(coro_io::to_flags(runtime_open_mode(std::ios::app)) ==
        coro_io::flags::append);
  CHECK(coro_io::to_flags(runtime_open_mode(std::ios::trunc)) ==
        coro_io::flags::truncate);
  CHECK(coro_io::to_flags(runtime_open_mode(std::ios::trunc | std::ios::out)) ==
        coro_io::flags::create_write_trunc);
  CHECK(coro_io::to_flags(runtime_open_mode(std::ios::in | std::ios::out |
                                            std::ios::trunc)) ==
        coro_io::flags::create_read_write_trunc);
  CHECK(coro_io::to_flags(
            runtime_open_mode(std::ios::in | std::ios::out | std::ios::app)) ==
        coro_io::flags::create_read_write_append);
}

TEST_CASE("sequential file reports failure states") {
  using seq_file =
      coro_io::basic_seq_coro_file<coro_io::execution_type::thread_pool>;

  seq_file unopened;
  CHECK_FALSE(unopened.seek(0, std::ios::beg));
  CHECK(unopened.get_execution_type() == coro_io::execution_type::none);

  test_file source("seq_failure.tmp", "content");
  seq_file file(source.path(), std::ios::in);
  REQUIRE(file.get_execution_type() == coro_io::execution_type::thread_pool);
  CHECK(file.open(source.path(), std::ios::in));

  auto write_result =
      async_simple::coro::syncAwait(file.async_write("cannot write"));
  CHECK(write_result.first == std::errc::io_error);
  CHECK(write_result.second == 0);

  auto previous_severity = easylog::get_min_severity();
  easylog::set_min_severity(easylog::Severity::WARN);
  seq_file missing;
  CHECK_FALSE(missing.open("missing_directory/file", std::ios::in));

  easylog::set_min_severity(easylog::Severity::INFO);
  CHECK_FALSE(missing.open("missing_directory/file", std::ios::in));
  easylog::set_min_severity(previous_severity);
}

TEST_CASE("random file reports invalid and failure states") {
  using random_file =
      coro_io::basic_random_coro_file<coro_io::execution_type::thread_pool>;

  random_file unopened;
  CHECK_FALSE(unopened.is_open());
  CHECK_FALSE(unopened.eof());
  CHECK(unopened.get_execution_type() == coro_io::execution_type::none);

  auto write_result =
      async_simple::coro::syncAwait(unopened.async_write_at(0, "x"));
  CHECK(write_result.first == std::errc::bad_file_descriptor);
  CHECK(write_result.second == 0);

  CHECK_FALSE(unopened.open("missing_directory/file", std::ios::in));

  coro_io::shared_file_handle invalid_handle;
  random_file invalid_file(invalid_handle);
  CHECK_FALSE(invalid_file.is_open());

  test_file source("random_failure.tmp", "content");
  random_file file;
  REQUIRE(file.open(source.path(), std::ios::in));
  CHECK(file.open(source.path(), std::ios::in));

  auto read_only_write =
      async_simple::coro::syncAwait(file.async_write_at(0, "x"));
  CHECK(read_only_write.first == std::errc::io_error);
  CHECK(read_only_write.second == 0);
}

TEST_CASE("shared file handle rejects invalid descriptors") {
  auto adopted = coro_io::shared_file_handle::adopt(-1);
  CHECK_FALSE(adopted.valid());

  auto negative_duplicate = coro_io::shared_file_handle::duplicate(-1);
  CHECK(negative_duplicate.first == std::errc::bad_file_descriptor);
  CHECK_FALSE(negative_duplicate.second.valid());

  auto closed_duplicate =
      coro_io::shared_file_handle::duplicate(std::numeric_limits<int>::max());
  CHECK(closed_duplicate.first == std::errc::bad_file_descriptor);
  CHECK_FALSE(closed_duplicate.second.valid());
}

TEST_CASE("shared file handle reports allocation failures") {
  std::pair<std::error_code, coro_io::shared_file_handle> open_path_failure;
  std::string long_path(128, 'x');
  {
    allocation_failure::fail_next_allocation failure;
    open_path_failure =
        coro_io::shared_file_handle::open(long_path, read_only_flag);
  }
  CHECK(open_path_failure.first == std::errc::not_enough_memory);
  CHECK_FALSE(open_path_failure.second.valid());

  test_file source("oom.tmp", "content");
  std::pair<std::error_code, coro_io::shared_file_handle> open_handle_failure;
  {
    allocation_failure::fail_next_allocation failure;
    open_handle_failure =
        coro_io::shared_file_handle::open(source.path(), read_only_flag);
  }
  CHECK(open_handle_failure.first == std::errc::not_enough_memory);
  CHECK_FALSE(open_handle_failure.second.valid());

  int adopted_fd = open_read_only(source.path());
  REQUIRE(adopted_fd >= 0);
  coro_io::shared_file_handle adopted;
  {
    allocation_failure::fail_next_allocation failure;
    adopted = coro_io::shared_file_handle::adopt(adopted_fd);
  }
  CHECK_FALSE(adopted.valid());
  CHECK(fd_is_open(adopted_fd));
  close_fd(adopted_fd);

  int source_fd = open_read_only(source.path());
  REQUIRE(source_fd >= 0);
  std::pair<std::error_code, coro_io::shared_file_handle> duplicate_failure;
  {
    allocation_failure::fail_next_allocation failure;
    duplicate_failure = coro_io::shared_file_handle::duplicate(source_fd);
  }
  CHECK(duplicate_failure.first == std::errc::not_enough_memory);
  CHECK_FALSE(duplicate_failure.second.valid());
  CHECK(fd_is_open(source_fd));
  close_fd(source_fd);
}

TEST_CASE("random file reports state allocation failure") {
  test_file source("state_oom.tmp", "content");
  auto open_result =
      coro_io::shared_file_handle::open(source.path(), read_only_flag);
  REQUIRE_FALSE(open_result.first);
  auto handle = std::move(open_result.second);

  asio::io_context io_context;
  {
    allocation_failure::fail_next_allocation failure;
    coro_io::basic_random_coro_file<coro_io::execution_type::thread_pool> file(
        handle, io_context.get_executor());
    CHECK_FALSE(file.is_open());
  }
  CHECK(handle.valid());
}

}  // namespace
