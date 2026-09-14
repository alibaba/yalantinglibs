#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <async_simple/coro/Collect.h>
#include <async_simple/coro/SyncAwait.h>
#include <doctest.h>

#include <chrono>
#include <future>
#include <latch>
#include <thread>
#include <ylt/coro_io/coro_file.hpp>

#include "file_io_fixture.hpp"

using coro_io::OwnRingIoContext;
using file_io_test::AlignedBuffer;
using file_io_test::block_size;
using file_io_test::TemporaryFile;
using Result = std::pair<std::error_code, size_t>;
using namespace std::chrono_literals;

static_assert(
    std::is_same_v<
        coro_io::coro_file,
        coro_io::basic_seq_coro_file<coro_io::execution_type::native_async>>);
#if defined(YLT_ENABLE_OWN_RING) && YLT_ENABLE_OWN_RING
static_assert(std::is_same_v<coro_io::random_coro_file,
                             coro_io::own_ring_random_coro_file>);
#else
static_assert(std::is_same_v<coro_io::random_coro_file,
                             coro_io::native_random_coro_file>);
#endif

namespace {

struct Hooks {
  Hooks() { OwnRingIoContext::fault() = {}; }
  ~Hooks() { OwnRingIoContext::fault() = {}; }
};

struct Gate {
  std::latch entered{1};
  std::latch released{1};
  std::atomic<bool> used{false};
  std::atomic<bool> opened{false};
  Gate() {
    OwnRingIoContext::fault().before_submit = [this] {
      if (!used.exchange(true)) {
        entered.count_down();
        released.wait();
      }
    };
  }
  void open() {
    if (!opened.exchange(true)) {
      released.count_down();
    }
  }
  ~Gate() { open(); }
};

template <typename Value>
Value wait(std::future<Value> &future) {
  REQUIRE(future.wait_for(10s) == std::future_status::ready);
  return future.get();
}

int read(OwnRingIoContext &context, int fd, char *buffer,
         unsigned length = block_size, uint64_t offset = 0) {
  std::promise<int> promise;
  auto future = promise.get_future();
  OwnRingIoContext::ReadRequest request;
  request.fd = fd;
  request.buf = buffer;
  request.len = length;
  request.offset = offset;
  request.on_done = [&](int result) {
    promise.set_value(result);
  };
  context.submit_borrowed_read(request);
  return wait(future);
}

}  // namespace

TEST_CASE("own ring modes initialize and stop on their owner") {
  for (auto mode :
       {OwnRingIoContext::Mode::plain, OwnRingIoContext::Mode::single_issuer,
        OwnRingIoContext::Mode::defer_taskrun}) {
    for (int attempt = 0; attempt < 30; ++attempt) {
      OwnRingIoContext::Options options;
      options.entries = 8;
      options.mode = mode;
      options.allow_fallback = false;
      OwnRingIoContext context(options);
      REQUIRE(context.ok());
      CHECK(context.defer_enabled() ==
            (mode == OwnRingIoContext::Mode::defer_taskrun));
      if (mode != OwnRingIoContext::Mode::plain) {
        CHECK((context.setup_flags() & IORING_SETUP_SINGLE_ISSUER) != 0);
      }
    }
  }
}

TEST_CASE("same fd requests retain identity across producers and a full SQ") {
  Hooks hooks;
  TemporaryFile file;
  Gate gate;
  OwnRingIoContext::Options options;
  options.entries = 8;
  options.max_requests = 1024;
  OwnRingIoContext context(options);
  REQUIRE(context.ok());
  gate.entered.wait();
  constexpr size_t count = 512;
  std::vector<OwnRingIoContext::ReadRequest> requests(count);
  std::vector<std::unique_ptr<AlignedBuffer>> buffers;
  std::vector<std::atomic<unsigned>> completions(count);
  buffers.reserve(count);
  std::atomic<size_t> remaining{count};
  std::atomic<size_t> errors{0};
  std::promise<bool> complete;
  auto future = complete.get_future();
  for (size_t index = 0; index < count; ++index) {
    buffers.emplace_back(std::make_unique<AlignedBuffer>());
    auto &request = requests[index];
    request.fd = file.fd();
    request.offset = (index % file.blocks()) * block_size;
    request.buf = buffers[index]->data();
    request.len = block_size;
    request.on_done = [&, index](int result) {
      if (result != block_size ||
          !file_io_test::verify(buffers[index]->data(), index % file.blocks(),
                                block_size)) {
        errors.fetch_add(1);
      }
      completions[index].fetch_add(1);
      if (remaining.fetch_sub(1) == 1) {
        complete.set_value(true);
      }
    };
  }
  std::vector<std::thread> producers;
  for (size_t producer = 0; producer < 8; ++producer) {
    producers.emplace_back([&, producer] {
      for (size_t index = producer; index < count; index += 8) {
        context.submit_borrowed_read(requests[index]);
      }
    });
  }
  for (auto &producer : producers) {
    producer.join();
  }
  gate.open();
  CHECK(wait(future));
  context.stop();
  CHECK(errors.load() == 0);
  for (auto &completion : completions) {
    CHECK(completion.load() == 1);
  }
  auto statistics = context.statistics();
  CHECK(statistics.accepted == count);
  CHECK(statistics.completed == count);
  CHECK(statistics.outstanding == 0);
  CHECK(statistics.max_inflight > 1);
  CHECK(statistics.wake_writes < count / 4);
}

TEST_CASE("bounded admission and shutdown complete each accepted request") {
  Hooks hooks;
  TemporaryFile file;
  Gate gate;
  OwnRingIoContext::Options options;
  options.max_requests = 2;
  OwnRingIoContext context(options);
  REQUIRE(context.ok());
  gate.entered.wait();
  std::array<OwnRingIoContext::ReadRequest, 3> requests;
  std::array<std::promise<int>, 3> promises;
  std::vector<std::future<int>> futures;
  AlignedBuffer buffer;
  for (size_t index = 0; index < requests.size(); ++index) {
    futures.push_back(promises[index].get_future());
    requests[index].fd = file.fd();
    requests[index].buf = buffer.data();
    requests[index].len = block_size;
    requests[index].on_done = [&, index](int result) {
      promises[index].set_value(result);
    };
    context.submit_borrowed_read(requests[index]);
  }
  CHECK(wait(futures[2]) == -EAGAIN);
  context.request_stop();
  gate.open();
  context.stop();
  CHECK(wait(futures[0]) == -ECANCELED);
  CHECK(wait(futures[1]) == -ECANCELED);
  CHECK(context.statistics().outstanding == 0);
  CHECK(read(context, file.fd(), buffer.data()) == -ECANCELED);
}

TEST_CASE("short scalar and vectored reads continue without mutating iovecs") {
  Hooks hooks;
  TemporaryFile file;
  OwnRingIoContext context;
  REQUIRE(context.ok());
  AlignedBuffer buffer(2 * block_size);
  std::atomic<int> calls{0};
  OwnRingIoContext::fault().map_res = [&](int result, const auto &) {
    return calls.fetch_add(1) == 0 ? 1024 : result;
  };
  CHECK(read(context, file.fd(), buffer.data()) == block_size);
  CHECK(file_io_test::verify(buffer.data(), 0, block_size));
  calls = 0;
  iovec vectors[] = {{buffer.data(), block_size},
                     {buffer.data() + block_size, block_size}};
  OwnRingIoContext::ReadRequest request;
  request.fd = file.fd();
  request.iov = vectors;
  request.iovcnt = 2;
  std::promise<int> promise;
  auto future = promise.get_future();
  request.on_done = [&](int result) {
    promise.set_value(result);
  };
  context.submit_borrowed_read(request);
  CHECK(wait(future) == 2 * block_size);
  CHECK(file_io_test::verify(buffer.data(), 0, 2 * block_size));
  CHECK(vectors[0].iov_base == buffer.data());
  CHECK(vectors[0].iov_len == block_size);
  context.stop();
}

TEST_CASE("initialization errors invalid input and transient completions") {
  Hooks hooks;
  TemporaryFile file;
  AlignedBuffer buffer;
  OwnRingIoContext::fault().force_init_fail = true;
  {
    OwnRingIoContext context;
    CHECK_FALSE(context.ok());
    CHECK(read(context, file.fd(), buffer.data()) == -EIO);
  }
  OwnRingIoContext::fault().force_init_fail = false;
  OwnRingIoContext context;
  REQUIRE(context.ok());
  CHECK(read(context, -1, buffer.data()) == -EBADF);
  CHECK(read(context, file.fd(), nullptr, 1) == -EINVAL);
  CHECK(read(context, file.fd(), buffer.data(), UINT_MAX) == -EOVERFLOW);
  CHECK(read(context, file.fd(), buffer.data(), 1, UINT64_MAX) == -EOVERFLOW);
  CHECK(read(context, file.fd(), nullptr, 0) == 0);
  int retries = 0;
  OwnRingIoContext::fault().map_res = [&](int result, const auto &) {
    return retries++ < 2 ? -EAGAIN : result;
  };
  CHECK(read(context, file.fd(), buffer.data()) == block_size);
  OwnRingIoContext::fault().map_res = [](int, const auto &) {
    return -EINTR;
  };
  CHECK(read(context, file.fd(), buffer.data()) == -EIO);
  context.stop();
  OwnRingIoContext::fault().map_res = {};
  OwnRingIoContext::fault().map_submit_rc = [](int) {
    return -EAGAIN;
  };
  OwnRingIoContext transient_context;
  REQUIRE(transient_context.ok());
  CHECK(read(transient_context, file.fd(), buffer.data()) == block_size);
  transient_context.stop();
}

TEST_CASE("cancellation waits for completion before releasing borrowed state") {
  Hooks hooks;
  TemporaryFile file;
  OwnRingIoContext context;
  REQUIRE(context.ok());
  AlignedBuffer buffer;
  auto token = std::make_shared<std::atomic<bool>>(false);
  OwnRingIoContext::fault().map_res = [token](int result, const auto &) {
    token->store(true);
    return result;
  };
  OwnRingIoContext::ReadRequest request;
  request.fd = file.fd();
  request.buf = buffer.data();
  request.len = block_size;
  request.cancellation = token;
  std::promise<int> promise;
  auto future = promise.get_future();
  request.on_done = [&](int result) {
    promise.set_value(result);
  };
  context.submit_borrowed_read(request);
  CHECK(wait(future) == -ECANCELED);
  CHECK(file_io_test::verify(buffer.data(), 0, block_size));
  context.stop();
  CHECK(context.statistics().outstanding == 0);
}

TEST_CASE("throwing callbacks do not kill the owner") {
  TemporaryFile file;
  OwnRingIoContext context;
  REQUIRE(context.ok());
  AlignedBuffer buffer;
  OwnRingIoContext::ReadRequest request;
  request.fd = file.fd();
  request.on_done = [](int) {
    throw std::runtime_error("callback");
  };
  context.submit_read(std::move(request));
  CHECK(read(context, file.fd(), buffer.data()) == block_size);
  context.stop();
  CHECK(context.statistics().completed == 2);
}

TEST_CASE(
    "default backend switch preserves writes and shared handle ownership") {
  TemporaryFile data;
  asio::io_context executor_context;
  coro_io::ExecutorWrapper executor(executor_context.get_executor());
  auto work = asio::make_work_guard(executor_context);
  std::jthread runner([&] {
    executor_context.run();
  });
  struct Stop {
    asio::io_context &context;
    ~Stop() { context.stop(); }
  } stop{executor_context};
  AlignedBuffer buffer;
  file_io_test::fill(buffer.data(), 0, block_size);
  auto exercise = [&](auto &file) {
    REQUIRE(file.is_open());
    auto written = async_simple::coro::syncAwait(
        file.async_write_at(0, {buffer.data(), block_size}));
    CHECK_FALSE(written.first);
    CHECK(written.second == block_size);
    std::memset(buffer.data(), 0, block_size);
    auto result = async_simple::coro::syncAwait(
        file.async_read_at(0, buffer.data(), block_size));
    CHECK_FALSE(result.first);
    CHECK(result.second == block_size);
    CHECK(file_io_test::verify(buffer.data(), 0, block_size));
  };
  coro_io::random_coro_file selected(&executor);
  REQUIRE(selected.open(data.path(), std::ios::in | std::ios::out));
  CHECK(selected.get_execution_type() ==
        coro_io::default_random_file_execution);
  exercise(selected);
  selected.close();
  CHECK(selected.get_execution_type() == coro_io::execution_type::none);
  auto closed = async_simple::coro::syncAwait(
      selected.async_read_at(0, buffer.data(), block_size));
  CHECK(closed.first == std::errc::bad_file_descriptor);
  auto [error, handle] = coro_io::shared_file_handle::open(data.path(), O_RDWR);
  REQUIRE_FALSE(error);
  coro_io::own_ring_random_coro_file own(handle, &executor);
  coro_io::native_random_coro_file native(handle, &executor);
  exercise(own);
  own.close();
  CHECK(handle.valid());
  exercise(native);
}

TEST_CASE("coro file holds fd until completion and reports EOF consistently") {
  Hooks hooks;
  TemporaryFile data;
  Gate gate;
  OwnRingIoContext context;
  REQUIRE(context.ok());
  asio::io_context executor_context;
  coro_io::ExecutorWrapper executor(executor_context.get_executor());
  coro_io::own_ring_random_coro_file file(context, &executor);
  REQUIRE(file.open(data.path(), std::ios::in, true));
  gate.entered.wait();
  AlignedBuffer buffer;
  std::promise<Result> promise;
  auto future = promise.get_future();
  file.async_read_at(0, buffer.data(), block_size)
      .start([&](async_simple::Try<Result> result) {
        promise.set_value(result.value());
      });
  file.close();
  gate.open();
  auto result = wait(future);
  CHECK_FALSE(result.first);
  CHECK(result.second == block_size);
  CHECK(file_io_test::verify(buffer.data(), 0, block_size));
  REQUIRE(file.open(data.path(), std::ios::in, false));
  result = async_simple::coro::syncAwait(
      file.async_read_at(data.blocks() * block_size - 64, buffer.data(), 128));
  CHECK_FALSE(result.first);
  CHECK(file.eof());
  CHECK(result.second == 64);
  iovec vectors[] = {{buffer.data(), 64}, {buffer.data() + 64, 64}};
  result = async_simple::coro::syncAwait(
      file.async_readv_at(data.blocks() * block_size - 64, vectors, 2));
  CHECK_FALSE(result.first);
  CHECK(result.second == 64);
  result = async_simple::coro::syncAwait(file.async_read_at(0, nullptr, 0));
  CHECK_FALSE(result.first);
  CHECK(result.second == 0);
}

TEST_CASE(
    "a borrowed request can resubmit from its completion without waking "
    "itself") {
  TemporaryFile file;
  OwnRingIoContext context;
  REQUIRE(context.ok());
  AlignedBuffer buffer;
  OwnRingIoContext::ReadRequest request;
  request.fd = file.fd();
  request.buf = buffer.data();
  request.len = block_size;
  size_t remaining = 1000;
  std::promise<bool> promise;
  auto future = promise.get_future();
  std::function<void(int)> continuation;
  continuation = [&](int result) {
    if (result != block_size ||
        !file_io_test::verify(buffer.data(), 0, block_size)) {
      promise.set_value(false);
      return;
    }
    if (--remaining == 0) {
      promise.set_value(true);
      return;
    }
    request.on_done = continuation;
    context.submit_borrowed_read(request);
  };
  request.on_done = continuation;
  context.submit_borrowed_read(request);
  CHECK(wait(future));
  context.stop();
  CHECK(context.statistics().completed == 1000);
  CHECK(context.statistics().wake_writes <= 2);
}

TEST_CASE(
    "stop during a completion drains other in-flight requests on the owner") {
  Hooks hooks;
  TemporaryFile file;
  Gate gate;
  OwnRingIoContext context;
  REQUIRE(context.ok());
  gate.entered.wait();
  constexpr size_t count = 32;
  std::array<OwnRingIoContext::ReadRequest, count> requests;
  std::vector<std::unique_ptr<AlignedBuffer>> buffers;
  std::atomic<size_t> remaining{count};
  std::atomic<size_t> errors{0};
  std::promise<bool> promise;
  auto future = promise.get_future();
  for (size_t index = 0; index < count; ++index) {
    buffers.emplace_back(std::make_unique<AlignedBuffer>());
    requests[index].fd = file.fd();
    requests[index].buf = buffers.back()->data();
    requests[index].len = block_size;
    requests[index].on_done = [&](int result) {
      context.request_stop();
      if (result != block_size && result != -ECANCELED) {
        errors.fetch_add(1);
      }
      if (remaining.fetch_sub(1) == 1) {
        promise.set_value(true);
      }
    };
    context.submit_borrowed_read(requests[index]);
  }
  gate.open();
  CHECK(wait(future));
  context.stop();
  CHECK(errors.load() == 0);
  CHECK(context.statistics().outstanding == 0);
  CHECK(context.statistics().max_inflight == count);
}

TEST_CASE("coro file cancellation signals skip queued IO") {
  Hooks hooks;
  TemporaryFile data;
  Gate gate;
  OwnRingIoContext context;
  REQUIRE(context.ok());
  gate.entered.wait();
  asio::io_context executor_context;
  coro_io::ExecutorWrapper executor(executor_context.get_executor());
  coro_io::own_ring_random_coro_file file(context, &executor);
  REQUIRE(file.open(data.path(), std::ios::in));
  auto signal = async_simple::Signal::create();
  AlignedBuffer buffer;
  std::promise<Result> promise;
  auto future = promise.get_future();
  file.async_read_at(0, buffer.data(), block_size)
      .setLazyLocal(signal.get())
      .start([&](async_simple::Try<Result> result) {
        if (result.hasError()) {
          promise.set_exception(result.getException());
        }
        else {
          promise.set_value(result.value());
        }
      });
  signal->emits(async_simple::SignalType::Terminate);
  gate.open();
  auto result = wait(future);
  CHECK(result.first == std::errc::operation_canceled);
  CHECK(result.second == 0);
  CHECK(context.statistics().read_sqes == 0);
  context.stop();
  result = async_simple::coro::syncAwait(
      file.async_read_at(0, buffer.data(), block_size)
          .setLazyLocal(signal.get()));
  CHECK(result.first == std::errc::operation_canceled);
}

TEST_CASE("explicit post resume runs the continuation on its executor") {
  TemporaryFile data;
  OwnRingIoContext::Options options;
  options.post_resume = true;
  OwnRingIoContext context(options);
  REQUIRE(context.ok());
  asio::io_context executor_context;
  coro_io::ExecutorWrapper executor(executor_context.get_executor());
  auto guard = asio::make_work_guard(executor_context);
  std::thread runner([&] {
    executor_context.run();
  });
  coro_io::own_ring_random_coro_file file(context, &executor);
  REQUIRE(file.open(data.path(), std::ios::in));
  AlignedBuffer buffer;
  std::promise<bool> promise;
  auto future = promise.get_future();
  file.async_read_at(0, buffer.data(), block_size)
      .start([&](async_simple::Try<Result> result) {
        promise.set_value(!result.hasError() && !result.value().first &&
                          std::this_thread::get_id() == runner.get_id());
      });
  bool passed = wait(future);
  guard.reset();
  executor_context.stop();
  runner.join();
  CHECK(passed);
}

TEST_CASE("setup fallback reports actual flags and strict mode fails") {
  Hooks hooks;
  OwnRingIoContext::fault().reject_optimized_setup = true;
  OwnRingIoContext::Options options;
  {
    OwnRingIoContext context(options);
    REQUIRE(context.ok());
    CHECK_FALSE(context.defer_enabled());
    CHECK(context.setup_flags() == 0);
  }
  options.allow_fallback = false;
  OwnRingIoContext context(options);
  CHECK_FALSE(context.ok());
  CHECK(context.init_error() == -EINVAL);
}

TEST_CASE("poll and spin modes service reads and shut down") {
  TemporaryFile file;
  for (bool poll : {false, true}) {
    OwnRingIoContext::Options options;
    options.poll = poll;
    options.spin_us = 10;
    OwnRingIoContext context(options);
    REQUIRE(context.ok());
    AlignedBuffer buffer;
    for (unsigned iteration = 0; iteration < 100; ++iteration) {
      CHECK(read(context, file.fd(), buffer.data()) == block_size);
    }
    context.stop();
    CHECK(context.statistics().outstanding == 0);
  }
}
