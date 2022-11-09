#pragma once
#include "data_def.hpp"
#include "ScopedTimer.hpp"
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


template<typename Data, typename Buffer>
struct Sample<SampleName::STRUCT_PACK, Data, Buffer>: public SampleBase {
  Sample(Data data): data_(std::move(data)) {}

  void clear_data() override {
    if constexpr (std::same_as<Data, std::vector<Monster>>) {
      data_.clear();
    }
  }
  void clear_buffer() override {
    buffer_.clear();
  }
  void reserve_buffer() override {
    buffer_.reserve(struct_pack::get_needed_size(data_));
  }
  size_t buffer_size() const override { return buffer_.size(); }
  std::string name() const override { return "struct_pack"; }
  void do_serialization(int run_idx) override {
    ScopedTimer timer(("serialize " + name()).c_str(), serialize_cost_[run_idx]);
    for (int i = 0; i < SAMPLES_COUNT; ++i) {
      struct_pack::serialize_to(buffer_, data_);
    }
  }
  void do_deserialization(int run_idx) override {
    ScopedTimer timer(("deserialize " + name()).c_str(), deserialize_cost_[run_idx]);
    for (int i = 0; i < SAMPLES_COUNT; ++i) {
      std::size_t len = 0;
      auto ec = struct_pack::deserialize_to_with_offset(data_, buffer_, len);
      if (ec != struct_pack::errc{}) [[unlikely]] {
        std::exit(1);
      }
    }
  }
 private:
  Data data_;
  Buffer buffer_;
};
template<SampleType sample_type>
auto create_sample() {
  using Buffer = std::string;
  if constexpr (sample_type == SampleType::RECT) {
    using Data = rect<int32_t>;
    auto rects = create_rects(OBJECT_COUNT);
    return Sample<SampleName::STRUCT_PACK, Data, Buffer>(rects[0]);
  }
  else if constexpr (sample_type == SampleType::RECTS) {
    using Data = std::vector<rect<int32_t>>;
    auto rects = create_rects(OBJECT_COUNT);
    return Sample<SampleName::STRUCT_PACK, Data, Buffer>(rects);
  }
  else if constexpr (sample_type == SampleType::PERSON) {
    using Data = person;
    auto persons = create_persons(OBJECT_COUNT);
    auto person = persons[0];
    return Sample<SampleName::STRUCT_PACK, Data, Buffer>(person);
  }
  else if constexpr (sample_type == SampleType::PERSONS) {
    using Data = std::vector<person>;
    auto persons = create_persons(OBJECT_COUNT);
    return Sample<SampleName::STRUCT_PACK, Data, Buffer>(persons);
  }
  else if constexpr (sample_type == SampleType::MONSTER) {
    using Data = Monster;
    auto monsters = create_monsters(OBJECT_COUNT);
    auto monster = monsters[0];
    return Sample<SampleName::STRUCT_PACK, Data, Buffer>(monster);
  }
  else if constexpr (sample_type == SampleType::MONSTERS) {
    using Data = std::vector<Monster>;
    auto monsters = create_monsters(OBJECT_COUNT);
    return Sample<SampleName::STRUCT_PACK, Data, Buffer>(monsters);
  }
  else {
    return sample_type;
  }
}
