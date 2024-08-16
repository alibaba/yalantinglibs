#include <array>
#include <atomic>
#include <chrono>
#include <iostream>
#include <random>
#include <thread>

#include "ylt/metric.hpp"

using namespace std::chrono_literals;
using namespace ylt::metric;

void bench_static_counter_qps(size_t thd_num, std::chrono::seconds duration,
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

void bench_dynamic_counter_qps(size_t thd_num, std::chrono::seconds duration,
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

void bench_many_metric_serialize(size_t COUNT, size_t LABEL_COUNT,
                                 bool to_json = false) {
  std::vector<std::shared_ptr<metric_t>> vec;
  for (size_t i = 0; i < COUNT; i++) {
    auto counter = std::make_shared<dynamic_counter_t>(
        std::string("qps"), "", std::array<std::string, 2>{"url", "code"});
    for (size_t j = 0; j < LABEL_COUNT; j++) {
      counter->inc({"test_label_value", std::to_string(j)});
    }
    vec.push_back(counter);
  }

  std::cout << "begin test\n";

  std::string str;

  auto start = std::chrono::system_clock::now();
  if (to_json) {
    str = manager_helper::serialize_to_json(vec);
  }
  else {
    str = manager_helper::serialize(vec);
  }

  auto end = std::chrono::system_clock::now();
  auto elaps =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
          .count();
  std::cout << "string size: " << str.size() << ", " << elaps << "ms\n";
}

void bench_many_labels_serialize(size_t COUNT, bool to_json = false) {
  dynamic_counter_t counter("qps2", "", {"url", "code"});
  std::string val(36, ' ');
  for (size_t i = 0; i < COUNT; i++) {
    strcpy(val.data(), std::to_string(i).data());
    counter.inc({"123e4567-e89b-12d3-a456-426614174000", val});
  }

  std::string str;
  auto start = std::chrono::system_clock::now();
  if (to_json) {
    counter.serialize_to_json(str);
  }
  else {
    counter.serialize(str);
  }

  auto end = std::chrono::system_clock::now();
  auto elaps =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
          .count();
  std::cout << elaps << "ms\n";
  std::cout << "label value count: " << counter.label_value_count()
            << " string size: " << str.size() << "\n";
}

void bench_many_labels_qps_summary(size_t thd_num,
                                   std::chrono::seconds duration) {
  dynamic_summary_2 summary(
      "qps2", "",
      summary_t::Quantiles{
          {0.5, 0.05}, {0.9, 0.01}, {0.95, 0.005}, {0.99, 0.001}},
      std::array<std::string, 2>{"method", "url"});
  std::atomic<bool> stop = false;
  std::vector<std::thread> vec;
  std::array<std::string, 2> arr{"/test", "200"};
  thread_local_value<uint64_t> local_val(thd_num);
  auto start = std::chrono::system_clock::now();
  std::string val(36, ' ');
  for (size_t i = 0; i < thd_num; i++) {
    vec.push_back(std::thread([&, i] {
      while (!stop) {
        strcpy(val.data(), std::to_string(i).data());
        summary.observe({"/test", std::to_string(get_random())},
                        get_random(100));
        local_val.inc();
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

  auto qps = local_val.value() / seconds;
  std::cout << "thd num: " << thd_num << ", qps: " << (int64_t)qps << "\n";
  for (auto& thd : vec) {
    thd.join();
  }

  start = std::chrono::system_clock::now();
  size_t last = summary.size_approx();
  std::cout << "total size: " << last << "\n";
  while (true) {
    std::this_thread::sleep_for(1s);
    size_t current = summary.size_approx();
    if (current == 0) {
      break;
    }

    std::cout << last - current << "\n";
    last = current;
  }
  end = std::chrono::system_clock::now();
  elaps = std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
              .count();
  std::cout << "consume " << elaps << "ms\n";
}

void bench_many_labels_serialize_summary(size_t COUNT, bool to_json = false) {
  dynamic_summary_2 summary(
      "qps2", "",
      summary_t::Quantiles{
          {0.5, 0.05}, {0.9, 0.01}, {0.95, 0.005}, {0.99, 0.001}},
      std::array<std::string, 2>{"method", "url"});
  std::string val(36, ' ');
  for (size_t i = 0; i < COUNT; i++) {
    strcpy(val.data(), std::to_string(i).data());
    summary.observe({"123e4567-e89b-12d3-a456-426614174000", val},
                    get_random(100));
  }

  std::string str;
  auto start = std::chrono::system_clock::now();
  if (to_json) {
    async_simple::coro::syncAwait(summary.serialize_to_json_async(str));
  }
  else {
    async_simple::coro::syncAwait(summary.serialize_async(str));
  }

  auto end = std::chrono::system_clock::now();
  auto elaps =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
          .count();
  std::cout << elaps << "ms\n";
  std::cout << "label value count: " << summary.label_value_count()
            << " string size: " << str.size() << "\n";
}

int main() {
  bench_many_labels_serialize_summary(100000);
  bench_many_labels_serialize_summary(1000000);

  bench_many_labels_serialize_summary(100000, true);
  bench_many_labels_serialize_summary(1000000, true);

  bench_many_labels_qps_summary(1, 5s);
  bench_many_labels_qps_summary(2, 5s);
  bench_many_labels_qps_summary(8, 5s);
  bench_many_labels_qps_summary(16, 5s);
  bench_many_labels_qps_summary(32, 5s);
  bench_many_labels_qps_summary(96, 5s);

  bench_many_labels_serialize(100000);
  bench_many_labels_serialize(1000000);
  bench_many_labels_serialize(10000000);
  bench_many_labels_serialize(100000, true);
  bench_many_labels_serialize(1000000, true);
  bench_many_labels_serialize(10000000, true);

  bench_many_metric_serialize(100000, 10);
  bench_many_metric_serialize(1000000, 10);
  bench_many_metric_serialize(100000, 10, true);
  bench_many_metric_serialize(1000000, 10, true);

  bench_static_counter_qps(1, 5s);
  bench_static_counter_qps(2, 5s);
  bench_static_counter_qps(8, 5s);
  bench_static_counter_qps(16, 5s);
  bench_static_counter_qps(32, 5s);
  bench_static_counter_qps(96, 5s);
  bench_static_counter_qps(32, 5s, 32);
  bench_static_counter_qps(96, 5s, 96);

  bench_dynamic_counter_qps(1, 5s);
  bench_dynamic_counter_qps(2, 5s);
  bench_dynamic_counter_qps(8, 5s);
  bench_dynamic_counter_qps(16, 5s);
  bench_dynamic_counter_qps(32, 5s);
  bench_dynamic_counter_qps(96, 10s);
  bench_dynamic_counter_qps(32, 5s, 32);
  bench_dynamic_counter_qps(96, 5s, 96);
}
