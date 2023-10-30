#include <async_simple/coro/Collect.h>
#include <doctest.h>

#include <ylt/coro_io/io_context_pool.hpp>
#include <ylt/coro_io/rate_limiter.hpp>

long currentTimeMills() {
  std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
  std::chrono::milliseconds ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(
          now.time_since_epoch());
  return ms.count();
}

TEST_CASE("test SmoothBurstyRateLimiter simple") {
  coro_io::SmoothBurstyRateLimiter rateLimiter(1);
  double wait_time = async_simple::coro::syncAwait(rateLimiter.acquire(1));
  CHECK_EQ(0, wait_time);
}

TEST_CASE("test SmoothBurstyRateLimiter acquire multi permits") {
  double permits_per_second = 10.0;
  int permits_to_acquire = 5;
  double expected_cost = (permits_to_acquire) * (1 / permits_per_second);
  double cost_diff = 0.1;

  coro_io::SmoothBurstyRateLimiter rateLimiter(permits_per_second);

  // acquire much enough, next acquire will wait
  async_simple::coro::syncAwait(rateLimiter.acquire(permits_to_acquire));

  long start_mills = currentTimeMills();
  async_simple::coro::syncAwait(rateLimiter.acquire(1));
  double cost = (currentTimeMills() - start_mills) / 1000.0;
  CHECK(cost > expected_cost - cost_diff);
  CHECK(cost < expected_cost + cost_diff);
}

TEST_CASE("test SmoothBurstyRateLimiter single thread") {
  double permits_per_second = 10.0;
  int permits_to_acquire = 5;
  double expected_cost = (permits_to_acquire - 1) * (1 / permits_per_second);
  double cost_diff = 0.1;

  long start_mills = currentTimeMills();
  coro_io::SmoothBurstyRateLimiter rateLimiter(permits_per_second);
  for (int i = 0; i < permits_to_acquire; i++) {
    double waitMills = async_simple::coro::syncAwait(rateLimiter.acquire(1));
    ELOG_INFO << "wait for " << waitMills;
  }
  double cost = (currentTimeMills() - start_mills) / 1000.0;

  CHECK(cost > expected_cost - cost_diff);
  CHECK(cost < expected_cost + cost_diff);
}

TEST_CASE("test SmoothBurstyRateLimiter multi coroutine") {
  double permits_per_second = 100.0;
  int num_of_coroutine = 5;
  int permits_to_acquire_every_coroutine = 5;
  double expected_cost =
      (num_of_coroutine * permits_to_acquire_every_coroutine - 1) *
      (1 / permits_per_second);
  double cost_diff = 0.1;

  coro_io::SmoothBurstyRateLimiter rateLimiter(permits_per_second);
  auto consumer = [&]() -> async_simple::coro::Lazy<void> {
    for (int i = 0; i < permits_to_acquire_every_coroutine; i++) {
      co_await rateLimiter.acquire(1);
    }
  };

  auto consumerListLazy = [&]() -> async_simple::coro::Lazy<void> {
    std::vector<async_simple::coro::Lazy<void>> lazyList;
    for (int i = 0; i < num_of_coroutine; i++) {
      lazyList.push_back(consumer());
    }
    co_await collectAllPara(std::move(lazyList));
  };

  long start_mills = currentTimeMills();
  syncAwait(consumerListLazy().via(
      coro_io::g_block_io_context_pool<coro_io::multithread_context_pool>(1)
          .get_executor()));
  double cost = (currentTimeMills() - start_mills) / 1000.0;

  CHECK(cost > expected_cost - cost_diff);
  CHECK(cost < expected_cost + cost_diff);
}