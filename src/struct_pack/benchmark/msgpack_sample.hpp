#pragma once
#include <cstring>
#include <memory>
#include <vector>

#include "ScopedTimer.hpp"
#include "data_def.hpp"
#include "no_op.h"
#include "sample.hpp"
struct tbuffer : public std::vector<char> {
  void write(const char *src, size_t sz) {
    this->resize(this->size() + sz);
    auto pos = this->data() + this->size() - sz;
    std::memcpy(pos, src, sz);
  }
};

struct message_pack_sample : public base_sample {
  std::string name() const override { return "message_pack"; }

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
        msgpack::pack(buffer_, sample);
      }
      no_op();
    }
    buf_size_map_.emplace(sample_type, buffer_.size());
  }

  void deserialize(SampleType sample_type, auto &sample) {
    using T = std::remove_cvref_t<decltype(sample)>;

    // get serialized buffer of sample for deserialize
    buffer_.clear();
    msgpack::pack(buffer_, sample);

    std::string bench_name =
        name() + " deserialize " + get_bench_name(sample_type);
    ScopedTimer timer(bench_name.data());
    for (int i = 0; i < SAMPLES_COUNT; ++i) {
      if constexpr (struct_pack::detail::container<T>) {
        sample.clear();
      }

      msgpack::unpacked unpacked;
      msgpack::unpack(unpacked, buffer_.data(), buffer_.size());
    }
    no_op();
  }

  std::vector<rect<int32_t>> rects_;
  std::vector<person> persons_;
  std::vector<Monster> monsters_;
  tbuffer buffer_;
};