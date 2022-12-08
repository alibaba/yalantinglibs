#pragma once
#include <stdexcept>
#include <type_traits>
#include <unordered_map>

#include "ScopedTimer.hpp"
#include "data_def.hpp"
#include "no_op.h"
#include "sample.hpp"
#include "struct_pack/struct_pack.hpp"

namespace struct_pack {
template <>
constexpr inline auto enable_type_info<person> = type_info_config::disable;
};

namespace struct_pack {
template <>
constexpr inline auto enable_type_info<std::vector<person>> =
    type_info_config::disable;
};

namespace struct_pack {
template <>
constexpr inline auto enable_type_info<rect<int32_t>> =
    type_info_config::disable;
};

namespace struct_pack {
template <>
constexpr inline auto enable_type_info<std::vector<rect<int32_t>>> =
    type_info_config::disable;
};

namespace struct_pack {
template <>
constexpr inline auto enable_type_info<Monster> = type_info_config::disable;
};

namespace struct_pack {
template <>
constexpr inline auto enable_type_info<std::vector<Monster>> =
    type_info_config::disable;
};

struct struct_pack_sample : public base_sample {
  std::string name() const override { return "struct_pack"; }

  void create_samples() override {
    rects_ = create_rects(OBJECT_COUNT);
    persons_ = create_persons(OBJECT_COUNT);
    monsters_ = create_monsters(OBJECT_COUNT);
  }

  void do_serialization(int run_idx) override {
    serialize(SampleType::RECT, rects_[0]);
    serialize(SampleType::RECTS, rects_);
    serialize(SampleType::PERSON, persons_[0]);
    serialize(SampleType::PERSONS, persons_);
    serialize(SampleType::MONSTER, monsters_[0]);
    serialize(SampleType::MONSTERS, monsters_);
  }

  void do_deserialization(int run_idx) override {
    deserialize(SampleType::RECT, rects_[0]);
    deserialize(SampleType::RECTS, rects_);
    deserialize(SampleType::PERSON, persons_[0]);
    deserialize(SampleType::PERSONS, persons_);
    deserialize(SampleType::MONSTER, monsters_[0]);
    deserialize(SampleType::MONSTERS, monsters_);
  }

 private:
  void serialize(SampleType sample_type, auto &sample) {
    {
      std::string bench_name =
          name() + " serialize " + get_bench_name(sample_type);
      ScopedTimer timer(bench_name.data());
      for (int i = 0; i < SAMPLES_COUNT; ++i) {
        buffer_.clear();
        struct_pack::serialize_to(buffer_, sample);
      }
      no_op();
    }
    buf_size_map_.emplace(sample_type, buffer_.size());
  }

  void deserialize(SampleType sample_type, auto &sample) {
    using T = std::remove_cvref_t<decltype(sample)>;

    // get serialized buffer of sample for deserialize
    buffer_.clear();
    struct_pack::serialize_to(buffer_, sample);

    std::string bench_name =
        name() + " deserialize " + get_bench_name(sample_type);
    ScopedTimer timer(bench_name.data());
    for (int i = 0; i < SAMPLES_COUNT; ++i) {
      if constexpr (struct_pack::detail::container<T>) {
        sample.clear();
      }

      [[maybe_unused]] auto ec = struct_pack::deserialize_to(sample, buffer_);
    }
    no_op();
  }

  std::vector<rect<int32_t>> rects_;
  std::vector<person> persons_;
  std::vector<Monster> monsters_;
  std::string buffer_;
};
