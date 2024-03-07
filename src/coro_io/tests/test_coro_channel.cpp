#include <doctest.h>

#include <array>
#include <chrono>
#include <iostream>
#include <ylt/coro_io/coro_io.hpp>
using namespace std::chrono_literals;

async_simple::coro::Lazy<void> test_coro_channel() {
  auto ch = coro_io::create_channel<int>(1000);

  co_await coro_io::async_send(ch, 41);
  co_await coro_io::async_send(ch, 42);

  std::error_code ec;
  int val;
  std::tie(ec, val) = co_await coro_io::async_receive(ch);
  CHECK(val == 41);

  std::tie(ec, val) = co_await coro_io::async_receive(ch);
  CHECK(val == 42);
}

async_simple::coro::Lazy<void> test_select_channel() {
  using namespace coro_io;
  auto ch1 = coro_io::create_channel<int>(1000);
  auto ch2 = coro_io::create_channel<int>(1000);

  co_await async_send(ch1, 41);
  co_await async_send(ch2, 42);

  std::array<int, 2> arr{41, 42};
  int val;
  size_t index = co_await select_t(ch1, ch2).on_recieve(
      [&](std::pair<std::error_code, int> result) {
        val = result.second;
      },
      [&](std::pair<std::error_code, int> result) {
        val = result.second;
      });
  CHECK(val == arr[index]);

  period_timer timer1(coro_io::get_global_executor());
  timer1.expires_after(100ms);
  period_timer timer2(coro_io::get_global_executor());
  timer2.expires_after(200ms);

  int val1;
  index = co_await select_t(timer1.async_await(), timer2.async_await())
              .on_recieve(
                  [&](bool timeout) {
                    CHECK(timeout);
                    val1 = 0;
                  },
                  [&](bool timeout) {
                    CHECK(timeout);
                    val1 = 1;
                  });
  CHECK(index == val1);

  int val2;
  index = co_await select_t(coro_io::post([] {
                            }),
                            coro_io::post([] {
                            }))
              .on_recieve(
                  [&]() {
                    std::cout << "post1\n";
                    val2 = 0;
                  },
                  [&]() {
                    std::cout << "post2\n";
                    val2 = 1;
                  });
  CHECK(index == val2);

  co_await async_send(ch1, 43);
  auto lazy = coro_io::post([] {
  });

  int val3 = -1;
  index = co_await select_t(ch1, lazy).on_recieve(
      [&](std::pair<std::error_code, int> result) {
        val3 = result.second;
      },
      [&] {
        val3 = 0;
      });

  if (index == 0) {
    CHECK(val3 == 43);
  }
  else if (index == 1) {
    CHECK(val3 == 0);
  }
}

TEST_CASE("test channel send recieve, test select channel and coroutine") {
  async_simple::coro::syncAwait(test_coro_channel());
  async_simple::coro::syncAwait(test_select_channel());
}