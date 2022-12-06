#pragma once

#include <stdint.h>

#include <algorithm>
#include <array>
#include <numeric>

#include "config.hpp"

template <SampleName sample_name, typename Data, typename Buffer>
struct Sample;

template <typename T>
inline uint64_t get_avg(const T& v) {
  uint64_t sum = std::accumulate(v.begin(), v.end(), uint64_t(0));
  return sum / v.size();
}
std::string pretty_int(int v) {
  auto str = std::to_string(v);
  for (int i = str.size(); i < 15; ++i) {
    str = ' ' + str;
  }
  return str;
}
std::string pretty_float(double v) {
  int vv = v * 100;
  std::string str{};
  str += std::to_string(vv / 100);
  str += ".";
  auto right = std::to_string(vv % 100);
  while (right.size() < 2) {
    right += '0';
  }
  str += right;
  for (int i = str.size(); i < 5; ++i) {
    str = ' ' + str;
  }
  return str;
}
std::string pretty_name(std::string name) {
  for (int i = name.size(); i < 12; ++i) {
    name += ' ';
  }
  return name;
}
struct SampleBase {
  virtual void clear_data() = 0;
  virtual void clear_buffer() = 0;
  virtual void reserve_buffer() = 0;
  virtual std::size_t buffer_size() const = 0;
  virtual std::string name() const = 0;
  virtual void do_serialization(int run_idx = 0) = 0;
  virtual void do_deserialization(int run_idx) = 0;
  void report_metric(std::string_view tag) const {
    std::cout << tag << " ";
    std::cout << pretty_name(name())
              << " serialize average: " << pretty_int(get_avg(serialize_cost_));
    std::cout << ", deserialize average: "
              << pretty_int(get_avg(deserialize_cost_));
    std::cout << ", buf size: " << pretty_int(buffer_size() / SAMPLES_COUNT);
    std::cout << std::endl;
  }
  std::array<uint64_t, RUN_COUNT> serialize_cost_{};
  std::array<uint64_t, RUN_COUNT> deserialize_cost_{};

 private:
};

template <typename T>
bool verify(const T& ground_truth, const T& t) {
  return ground_truth == t;
}
