#pragma once

#include <stdint.h>
#include <sys/types.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <numeric>

#include "config.hpp"

struct base_sample {
  virtual std::string name() const = 0;
  virtual void do_serialization(int run_idx = 0) = 0;
  virtual void do_deserialization(int run_idx) = 0;
  virtual void create_samples() {}
  virtual ~base_sample() {}

  void print_buffer_size() {
    for (auto sample_type : g_sample_type_vec) {
      std::cout << get_bench_name(sample_type)
                << " buffer size = " << buffer_size(sample_type) << "\n";
    }
  }

  virtual std::size_t buffer_size(SampleType type) const {
    auto it = buf_size_map_.find(type);
    if (it == buf_size_map_.end()) {
      throw std::runtime_error("unknown sample type");
    }

    return it->second;
  }

  auto get_ser_time_elapsed_map() { return ser_time_elapsed_map_; }
  auto get_deser_time_elapsed_map() { return deser_time_elapsed_map_; }

 protected:
  std::unordered_map<SampleType, size_t> buf_size_map_;
  std::unordered_map<SampleType, uint64_t> ser_time_elapsed_map_;
  std::unordered_map<SampleType, uint64_t> deser_time_elapsed_map_;
};
