#pragma once
#include <cstring>
#include <memory>
#include <vector>

#include "ScopedTimer.hpp"
#include "data_def.hpp"
#include "no_op.h"
#include "sample.hpp"
template <typename Data, typename Buffer>
struct Sample<SampleName::MSGPACK, Data, Buffer> : public SampleBase {
  Sample(Data data) : data_(std::move(data)) {
    msgpack::pack(buffer_, data_);
    size_ = buffer_.size();
  }
  void clear_data() override {
    if constexpr (std::same_as<Data, std::vector<Monster>>) {
      data_.clear();
    }
  }
  void clear_buffer() override { buffer_.clear(); }
  void reserve_buffer() override { buffer_.reserve(size_ * SAMPLES_COUNT); }
  size_t buffer_size() const override { return buffer_.size(); }
  std::string name() const override { return "msgpack"; }
  void do_serialization(int run_idx) override {
    ScopedTimer timer(("serialize " + name()).c_str(),
                      serialize_cost_[run_idx]);
    for (std::size_t i = 0; i < SAMPLES_COUNT; ++i) {
      msgpack::pack(buffer_, data_);
    }
    no_op();
  }
  void do_deserialization(int run_idx) override {
    msgpack::unpacked unpacked;
    ScopedTimer timer(("deserialize " + name()).c_str(),
                      deserialize_cost_[run_idx]);
    std::size_t pos = 0;
    for (std::size_t i = 0; i < SAMPLES_COUNT; ++i) {
      msgpack::unpack(unpacked, buffer_.data(), buffer_.size(), pos);
      data_ = unpacked.get().as<Data>();
    }
    no_op();
  }

 private:
  Data data_;
  Buffer buffer_;
  std::size_t size_;
};

struct tbuffer : public std::vector<char> {
  void write(const char *src, size_t sz) {
    this->resize(this->size() + sz);
    auto pos = this->data() + this->size() - sz;
    std::memcpy(pos, src, sz);
  }
};

namespace msgpack_sample {
template <SampleType sample_type>
auto create_sample() {
  using Buffer = tbuffer;
  if constexpr (sample_type == SampleType::RECT) {
    using Data = rect<int32_t>;
    auto rects = create_rects(OBJECT_COUNT);
    return Sample<SampleName::MSGPACK, Data, Buffer>(rects[0]);
  }
  else if constexpr (sample_type == SampleType::RECTS) {
    using Data = std::vector<rect<int32_t>>;
    auto rects = create_rects(OBJECT_COUNT);
    return Sample<SampleName::MSGPACK, Data, Buffer>(rects);
  }
  else if constexpr (sample_type == SampleType::PERSON) {
    using Data = person;
    auto persons = create_persons(OBJECT_COUNT);
    auto person = persons[0];
    return Sample<SampleName::MSGPACK, Data, Buffer>(person);
  }
  else if constexpr (sample_type == SampleType::PERSONS) {
    using Data = std::vector<person>;
    auto persons = create_persons(OBJECT_COUNT);
    return Sample<SampleName::MSGPACK, Data, Buffer>(persons);
  }
  else if constexpr (sample_type == SampleType::MONSTER) {
    using Data = Monster;
    auto monsters = create_monsters(OBJECT_COUNT);
    auto monster = monsters[0];
    return Sample<SampleName::MSGPACK, Data, Buffer>(monster);
  }
  else if constexpr (sample_type == SampleType::MONSTERS) {
    using Data = std::vector<Monster>;
    auto monsters = create_monsters(OBJECT_COUNT);
    return Sample<SampleName::MSGPACK, Data, Buffer>(monsters);
  }
  else {
    return sample_type;
  }
}
}  // namespace msgpack_sample
