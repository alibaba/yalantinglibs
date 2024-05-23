#pragma once
#include <ylt/struct_pb.hpp>

#include "ScopedTimer.hpp"
#include "no_op.h"
#include "sample.hpp"

namespace pb_sample {
struct person : public iguana::pb_base {
  int32_t id;
  std::string name;
  int age;
  double salary;
};
REFLECTION(person, id, name, age, salary);

struct persons : public iguana::pb_base {
  std::vector<person> list;
};
REFLECTION(persons, list);

struct rect : public iguana::pb_base {
  int32_t x = 1;
  int32_t y = 0;
  int32_t width = 11;
  int32_t height = 1;
};
REFLECTION(rect, x, y, width, height);

struct rects : public iguana::pb_base {
  std::vector<rect> list;
};
REFLECTION(rects, list);

struct Vec3 : public iguana::pb_base {
  float x;
  float y;
  float z;

  REFLECTION(Vec3, x, y, z);
};

struct Weapon : public iguana::pb_base {
  std::string name;
  int32_t damage;
};
REFLECTION(Weapon, name, damage);

enum Color : uint8_t { Red, Green, Blue };

struct Monster : public iguana::pb_base {
  Vec3 pos;
  int32_t mana;
  int32_t hp;
  std::string name;
  std::string inventory;
  Color color;
  std::vector<Weapon> weapons;
  Weapon equipped;
  std::vector<Vec3> path;
};
REFLECTION(Monster, pos, mana, hp, name, inventory, color, weapons, equipped,
           path);

struct Monsters : public iguana::pb_base {
  std::vector<Monster> list;
};
REFLECTION(Monsters, list);

inline auto create_rects(size_t object_count) {
  rect rc{{0}, 1, 0, 11, 1};
  std::vector<rect> v{};
  for (std::size_t i = 0; i < object_count; i++) {
    v.push_back(rc);
  }
  return v;
}

inline auto create_persons(size_t object_count) {
  std::vector<person> v{};
  person p{{0}, 432798, std::string(1024, 'A'), 24, 65536.42};

  for (std::size_t i = 0; i < object_count; i++) {
    v.push_back(p);
  }
  return v;
}

inline std::vector<Monster> create_monsters(size_t object_count) {
  std::vector<Monster> v{};
  Monster m = {
      0,
      Vec3{0, 1, 2, 3},
      16,
      24,
      "it is a test",
      "\1\2\3\4",
      Color::Red,
      {{0, "gun", 42}, {0, "shotgun", 56}},
      {0, "air craft", 67},
      {{0, 7, 8, 9}, {0, 71, 81, 91}},
  };

  Monster m1 = {
      0,
      {0, 11, 22, 33},
      161,
      241,
      "it is a test, ok",
      "\24\25\26\24",
      Color::Red,
      {{0, "gun", 421}, {0, "shotgun", 561}},
      {0, "air craft", 671},
      {{0, 71, 82, 93}, {0, 711, 821, 931}},
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
}  // namespace pb_sample

struct struct_pb_sample : public base_sample {
  static inline constexpr LibType lib_type = LibType::STRUCT_PB;
  std::string name() const override { return get_lib_name(lib_type); }

  void create_samples() override {
    rects_.list = {pb_sample::create_rects(OBJECT_COUNT)};
    persons_.list = {pb_sample::create_persons(OBJECT_COUNT)};
    monsters_.list = {pb_sample::create_monsters(OBJECT_COUNT)};
  }

  void do_serialization() override {
    serialize(SampleType::RECT, rects_.list[0]);
    struct_pb::to_pb(rects_, buffer_);
    struct_pb::to_pb(rects_, buffer_);
    struct_pb::to_pb(rects_, buffer_);
    std::cout << rects_.list.size() << "\n";
    serialize(SampleType::RECTS, rects_);
    serialize(SampleType::PERSON, persons_.list[0]);
    serialize(SampleType::PERSONS, persons_);
    serialize(SampleType::MONSTER, monsters_.list[0]);
    serialize(SampleType::MONSTERS, monsters_);
  }

  void do_deserialization() override {
    deserialize(SampleType::RECT, rects_.list[0]);
    deserialize(SampleType::RECTS, rects_);
    deserialize(SampleType::PERSON, persons_.list[0]);
    deserialize(SampleType::PERSONS, persons_);
    deserialize(SampleType::MONSTER, monsters_.list[0]);
    deserialize(SampleType::MONSTERS, monsters_);
  }

 private:
  template <typename T>
  void serialize(SampleType sample_type, T &sample) {
    {
      struct_pb::to_pb(sample, buffer_);
      buffer_.clear();

      uint64_t ns = 0;
      std::string bench_name =
          name() + " serialize " + get_sample_name(sample_type);

      {
        ScopedTimer timer(bench_name.data(), ns);
        for (int i = 0; i < ITERATIONS; ++i) {
          struct_pb::to_pb(sample, buffer_);
          no_op(buffer_);
          no_op((char *)&sample);
        }
      }
      ser_time_elapsed_map_.emplace(sample_type, ns);
    }
    buf_size_map_.emplace(sample_type, buffer_.size());
  }
  template <typename T, typename U = T>
  void deserialize(SampleType sample_type, T &sample) {
    buffer_.clear();
    struct_pb::to_pb(sample, buffer_);

    uint64_t ns = 0;
    std::string bench_name =
        name() + " deserialize " + get_sample_name(sample_type);

    {
      ScopedTimer timer(bench_name.data(), ns);
      for (int i = 0; i < ITERATIONS; ++i) {
        U obj;
        struct_pb::from_pb(obj, buffer_);
        no_op((char *)&obj);
        no_op(buffer_);
      }
    }
    deser_time_elapsed_map_.emplace(sample_type, ns);
  }

  pb_sample::rects rects_;
  pb_sample::persons persons_;
  pb_sample::Monsters monsters_;
  std::string buffer_;
};
