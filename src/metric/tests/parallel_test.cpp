#include <chrono>
#include <thread>

#include "../benchmark/bench.hpp"
#include "doctest.h"
#include "ylt/metric.hpp"

TEST_CASE("test high parallel perform test") {
  bench_static_summary_mixed(std::thread::hardware_concurrency() * 4, 3s);
  bench_dynamic_summary_mixed(std::thread::hardware_concurrency() * 4, 2s);
  bench_static_counter_mixed(std::thread::hardware_concurrency() * 4, 2s);
  bench_dynamic_counter_mixed(std::thread::hardware_concurrency() * 4, 2s);
}