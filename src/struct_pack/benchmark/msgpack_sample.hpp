#pragma once
#include <cstring>
#include <memory>
#include <vector>

#include "msgpack.hpp"

#include "ScopedTimer.hpp"
#include "config.hpp"
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
  static inline constexpr LibType lib_type = LibType::MSGPACK;
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
  void serialize(SampleType sample_type, auto &sample) {
    {
      msgpack::pack(buffer_, sample);
      buffer_.clear();

      uint64_t ns = 0;
      std::string bench_name =
          name() + " serialize " + get_sample_name(sample_type);

      {
        ScopedTimer timer(bench_name.data(), ns);
        for (int i = 0; i < ITERATIONS; ++i) {
          buffer_.clear();
          msgpack::pack(buffer_, sample);
          no_op(buffer_.data());
        }
      }
      ser_time_elapsed_map_.emplace(sample_type, ns);
    }
    buf_size_map_.emplace(sample_type, buffer_.size());
  }

  void deserialize(SampleType sample_type, auto &sample) {
    using T = std::remove_cvref_t<decltype(sample)>;

    // get serialized buffer of sample for deserialize
    buffer_.clear();
    msgpack::pack(buffer_, sample);

    std::vector<msgpack::unpacked> vec;
    vec.resize(ITERATIONS);

    uint64_t ns = 0;
    std::string bench_name =
        name() + " deserialize " + get_sample_name(sample_type);

    {
      ScopedTimer timer(bench_name.data(), ns);
      for (int i = 0; i < ITERATIONS; ++i) {
        msgpack::unpack(vec[i], buffer_.data(), buffer_.size());
        vec[i].get().as<T>();
      }
      no_op((char *)vec.data());
    }
    deser_time_elapsed_map_.emplace(sample_type, ns);
  }

  std::vector<rect<int32_t>> rects_;
  std::vector<person> persons_;
  std::vector<Monster> monsters_;
  tbuffer buffer_;
};