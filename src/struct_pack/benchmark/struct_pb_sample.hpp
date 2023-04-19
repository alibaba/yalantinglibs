#pragma once
#include <cassert>
#include <optional>
#include <stdexcept>
#include <valarray>

#include "ScopedTimer.hpp"
#include "data_def.struct_pb.h"
#include "no_op.h"
#include "sample.hpp"
namespace struct_pb_sample {

auto create_rects(std::size_t object_count) {
  rect32s rcs;
  for (int i = 0; i < object_count; ++i) {
    rect32 rc{65536, 65536, 65536, 65536};
    rcs.rect32_list.emplace_back(rc);
  }
  return rcs;
}
auto create_persons(std::size_t object_count) {
  persons ps;
  for (int i = 0; i < object_count; ++i) {
    person p{65536, "tom", 65536, 65536.42};
    ps.person_list.emplace_back(p);
  }
  return ps;
}
auto create_monsters(std::size_t object_count) {
  Monsters monsters;
  for (int i = 0; i < object_count / 2; ++i) {
    {
      Monster m{};
      m.pos = std::make_unique<struct_pb_sample::Vec3>();
      *m.pos = struct_pb_sample::Vec3{1, 2, 3};
      //      m.pos = {1, 2, 3};
      m.mana = 16;
      m.hp = 24;
      m.name = "it is a test";
      m.inventory = "\1\2\3\4";
      m.color = Monster::Color::Red;
      m.weapons = {{"gun", 42}, {"mission", 56}};
      m.equipped = std::make_unique<struct_pb_sample::Weapon>();
      *m.equipped = struct_pb_sample::Weapon{"air craft", 67};
      // m.equipped = {"air craft", 67};
      m.path = {{7, 8, 9}};
      monsters.monsters.push_back(std::move(m));
    }
    {
      Monster m;
      m.pos = std::make_unique<struct_pb_sample::Vec3>();
      *m.pos = struct_pb_sample::Vec3{11, 22, 33};
      // m.pos = {11, 22, 33};
      m.mana = 161;
      m.hp = 241;
      m.name = "it is a test, ok";
      m.inventory = "\24\25\26\24";
      m.color = Monster::Color::Red;
      m.weapons = {{"gun", 421}, {"mission", 561}};
      m.equipped = std::make_unique<struct_pb_sample::Weapon>();
      *m.equipped = struct_pb_sample::Weapon{"air craft", 671};
      // m.equipped = {"air craft", 671};
      m.path = {{71, 82, 93}};
      monsters.monsters.push_back(std::move(m));
    }
  }
  return monsters;
}

struct struct_pb_sample_t : public base_sample {
  static inline constexpr LibType lib_type = LibType::STRUCT_PB;
  std::string name() const override { return get_lib_name(lib_type); }

  void create_samples() override {
    rects_ = struct_pb_sample::create_rects(OBJECT_COUNT);
    persons_ = struct_pb_sample::create_persons(OBJECT_COUNT);
    monsters_ = struct_pb_sample::create_monsters(OBJECT_COUNT);
  }

  void do_serialization() override {
    serialize(SampleType::RECT, rects_.rect32_list[0]);
    serialize(SampleType::RECTS, rects_);
    serialize(SampleType::PERSON, persons_.person_list[0]);
    serialize(SampleType::PERSONS, persons_);
    serialize(SampleType::MONSTER, monsters_.monsters[0]);
    serialize(SampleType::MONSTERS, monsters_);
  }

  void do_deserialization() override {
    deserialize(SampleType::RECT, rects_.rect32_list[0]);
    deserialize(SampleType::RECTS, rects_);
    deserialize(SampleType::PERSON, persons_.person_list[0]);
    deserialize(SampleType::PERSONS, persons_);
    deserialize(SampleType::MONSTER, monsters_.monsters[0]);
    deserialize(SampleType::MONSTERS, monsters_);
  }

 private:
  void serialize(SampleType sample_type, auto& sample) {
    auto sz = struct_pb::internal::get_needed_size(sample);
    buffer_.resize(sz);
    struct_pb::internal::serialize_to(buffer_.data(), buffer_.size(), sample);
    buffer_.clear();
    buffer_.resize(sz);

    uint64_t ns = 0;
    std::string bench_name =
        name() + " serialize " + get_sample_name(sample_type);

    {
      ScopedTimer timer(bench_name.data(), ns);
      for (int i = 0; i < ITERATIONS; ++i) {
        buffer_.clear();
        auto sz = struct_pb::internal::get_needed_size(sample);
        buffer_.resize(sz);
        struct_pb::internal::serialize_to(buffer_.data(), buffer_.size(),
                                          sample);
        no_op(buffer_);
      }
    }

    ser_time_elapsed_map_.emplace(sample_type, ns);
    buf_size_map_.emplace(sample_type, buffer_.size());
  }

  void deserialize(SampleType sample_type, auto& sample) {
    using T = std::remove_cvref_t<decltype(sample)>;
    buffer_.clear();
    auto sz = struct_pb::internal::get_needed_size(sample);
    buffer_.resize(sz);
    struct_pb::internal::serialize_to(buffer_.data(), buffer_.size(), sample);

    std::vector<T> vec;
    vec.resize(ITERATIONS);

    uint64_t ns = 0;
    std::string bench_name =
        name() + " deserialize " + get_sample_name(sample_type);

    {
      ScopedTimer timer(bench_name.data(), ns);
      for (int i = 0; i < ITERATIONS; ++i) {
        [[maybe_unused]] auto ok = struct_pb::internal::deserialize_to(
            vec[i], buffer_.data(), buffer_.size());
        assert(ok);
      }
      no_op((char*)vec.data());
    }
    deser_time_elapsed_map_.emplace(sample_type, ns);

    buf_size_map_.emplace(sample_type, buffer_.size());
  }

  struct_pb_sample::rect32s rects_;
  struct_pb_sample::persons persons_;
  struct_pb_sample::Monsters monsters_;
  std::string buffer_;
};
}  // namespace struct_pb_sample