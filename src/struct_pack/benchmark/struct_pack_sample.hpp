#pragma once
#include <stdexcept>
#include <type_traits>
#include <unordered_map>
#include <ylt/struct_pack.hpp>

#include "ScopedTimer.hpp"
#include "config.hpp"
#include "data_def.hpp"
#include "no_op.h"
#include "sample.hpp"

inline auto create_rects(size_t object_count) {
  rect<int32_t> rc{1, 11, 1111, 111111};
  std::vector<rect<int32_t>> v{};
  for (std::size_t i = 0; i < object_count; i++) {
    v.push_back(rc);
  }
  return v;
}

inline auto create_persons(size_t object_count) {
  std::vector<person> v{};
  person p{432798, "tom", 24, 65536.42};
  for (std::size_t i = 0; i < object_count; i++) {
    v.push_back(p);
  }
  return v;
}

inline std::vector<Monster> create_monsters(size_t object_count) {
  std::vector<Monster> v{};
  Monster m = {
      {1, 2, 3},
      16,
      24,
      "it is a test",
      "\1\2\3\4",
      Color::Red,
      {{"gun", 42}, {"shotgun", 56}},
      {"air craft", 67},
      {{7, 8, 9}, {71, 81, 91}},
  };

  Monster m1 = {
      {11, 22, 33},
      161,
      241,
      "it is a test, ok",
      "\24\25\26\24",
      Color::Red,
      {{"gun", 421}, {"shotgun", 561}},
      {"air craft", 671},
      {{71, 82, 93}, {711, 821, 931}},
  };

  for (std::size_t i = 0; i < object_count / 2; i++) {
    v.push_back(m);
    v.push_back(m1);
  }

  if (object_count % 2 == 1) {
    v.push_back(m);
  }

  return v;
}

struct struct_pack_sample : public base_sample {
  static inline constexpr LibType lib_type = LibType::STRUCT_PACK;
  std::string name() const override { return get_lib_name(lib_type); }

  void create_samples() override {
    rects_ = create_rects(OBJECT_COUNT);
    persons_ = create_persons(OBJECT_COUNT);
    monsters_ = create_monsters(OBJECT_COUNT);
  }

  void do_serialization() override {
    serialize(SampleType::RECT, rects_[0]);
    serialize(SampleType::RECTS, rects_);
    serialize(SampleType::PERSON, persons_[0]);
    serialize(SampleType::PERSONS, persons_);
    serialize(SampleType::MONSTER, monsters_[0]);
    serialize(SampleType::MONSTERS, monsters_);
  }

  void do_deserialization() override {
    deserialize(SampleType::RECT, rects_[0]);
    deserialize(SampleType::RECTS, rects_);
    deserialize(SampleType::PERSON, persons_[0]);
    deserialize(SampleType::PERSONS, persons_);
    deserialize(SampleType::MONSTER, monsters_[0]);
    deserialize(SampleType::MONSTERS, monsters_);
  }

 private:
  template <typename T>
  void serialize(SampleType sample_type, T &sample) {
    {
      struct_pack::serialize_to(buffer_, sample);
      buffer_.clear();

      uint64_t ns = 0;
      std::string bench_name =
          name() + " serialize " + get_sample_name(sample_type);

      {
        ScopedTimer timer(bench_name.data(), ns);
        for (int i = 0; i < ITERATIONS; ++i) {
          buffer_.clear();
          struct_pack::serialize_to(buffer_, sample);
          no_op(buffer_);
        }
      }
      ser_time_elapsed_map_.emplace(sample_type, ns);
    }
    buf_size_map_.emplace(sample_type, buffer_.size());
  }
  template <typename T>
  void deserialize(SampleType sample_type, T &sample) {
    // get serialized buffer of sample for deserialize
    buffer_.clear();
    struct_pack::serialize_to(buffer_, sample);

    std::vector<T> vec;
    vec.resize(ITERATIONS);

    uint64_t ns = 0;
    std::string bench_name =
        name() + " deserialize " + get_sample_name(sample_type);

    {
      ScopedTimer timer(bench_name.data(), ns);
      for (int i = 0; i < ITERATIONS; ++i) {
        [[maybe_unused]] auto ec = struct_pack::deserialize_to(vec[i], buffer_);
      }
      no_op((char *)vec.data());
    }
    deser_time_elapsed_map_.emplace(sample_type, ns);
  }

  std::vector<rect<int32_t>> rects_;
  std::vector<person> persons_;
  std::vector<Monster> monsters_;
  std::string buffer_;
};
