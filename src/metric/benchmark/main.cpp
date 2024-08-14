#include <array>
#include <atomic>
#include <chrono>
#include <iostream>
#include <random>
#include <thread>

#include "ylt/metric.hpp"

using namespace std::chrono_literals;
using namespace ylt::metric;

void bench_static_counter(size_t thd_num, std::chrono::seconds duration,
                          size_t dupli_count = 2) {
  counter_t counter("qps", "", dupli_count);
  std::vector<std::thread> vec;
  std::atomic<bool> stop = false;
  auto start = std::chrono::system_clock::now();
  for (size_t i = 0; i < thd_num; i++) {
    vec.push_back(std::thread([&] {
      while (!stop) {
        counter.inc(1);
      }
    }));
  }

  std::this_thread::sleep_for(duration);
  stop = true;
  auto end = std::chrono::system_clock::now();

  auto elaps =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
          .count();

  double seconds = double(elaps) / 1000;

  auto qps = counter.value() / seconds;
  std::cout << "duplicate count: " << dupli_count << ", thd num: " << thd_num
            << ", qps: " << (uint64_t)qps << "\n";

  for (auto& thd : vec) {
    thd.join();
  }
}

auto get_random(size_t range = 10000) {
  thread_local std::default_random_engine gen(std::time(nullptr));
  std::uniform_int_distribution<> distr(1, range);
  return distr(gen);
}

void bench_dynamic_counter(size_t thd_num, std::chrono::seconds duration,
                           size_t dupli_count = 2) {
  dynamic_counter_t counter("qps2", "", {"url", "code"}, dupli_count);
  std::atomic<bool> stop = false;
  std::vector<std::thread> vec;
  std::array<std::string, 2> arr{"/test", "200"};
  auto start = std::chrono::system_clock::now();
  for (size_t i = 0; i < thd_num; i++) {
    vec.push_back(std::thread([&, i] {
      while (!stop) {
        counter.inc({"/test", std::to_string(get_random())}, 1);
      }
    }));
  }
  std::this_thread::sleep_for(duration);
  stop = true;
  auto end = std::chrono::system_clock::now();

  auto elaps =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
          .count();

  double seconds = double(elaps) / 1000;
  std::cout << "run " << elaps << "ms, " << seconds << " seconds\n";

  size_t total = 0;
  for (size_t i = 0; i < 10000; i++) {
    total += counter.value({"/test", std::to_string(i)});
  }
  auto qps = total / seconds;
  std::cout << "duplicate count: " << dupli_count << ", thd num: " << thd_num
            << ", qps: " << (int64_t)qps << "\n";
  for (auto& thd : vec) {
    thd.join();
  }
}

int main() {
  bench_static_counter(1, 5s);
  bench_static_counter(2, 5s);
  bench_static_counter(8, 5s);
  bench_static_counter(16, 5s);
  bench_static_counter(32, 5s);
  bench_static_counter(96, 5s);
  bench_static_counter(32, 5s, 32);
  bench_static_counter(96, 5s, 96);

  bench_dynamic_counter(1, 5s);
  bench_dynamic_counter(2, 5s);
  bench_dynamic_counter(8, 5s);
  bench_dynamic_counter(16, 5s);
  bench_dynamic_counter(32, 5s);
  bench_dynamic_counter(96, 10s);
  bench_dynamic_counter(32, 5s, 32);
  bench_dynamic_counter(96, 5s, 96);
}
