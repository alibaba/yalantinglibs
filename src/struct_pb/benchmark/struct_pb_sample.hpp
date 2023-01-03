#pragma once
#include <optional>
#include <valarray>

#include "ScopedTimer.hpp"
#include "benchmark.struct_pb.h"
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
template <typename T>
bool check_unique_ptr(const T& lhs, const T& rhs) {
  if (lhs && rhs) {
    return *lhs == *rhs;
  }
  return !lhs && !rhs;
}
bool operator==(const Vec3& lhs, const Vec3& rhs) {
  std::valarray<float> lh({lhs.x, lhs.y, lhs.z});
  std::valarray<float> rh({rhs.x, rhs.y, rhs.z});
  return (std::abs(lh - rh) < 0.05f).min();
}
bool operator==(const Weapon& lhs, const Weapon& rhs) {
  return lhs.name == rhs.name && lhs.damage == rhs.damage;
};
bool operator==(const rect32& lhs, const rect32& rhs) {
  return lhs.x == rhs.x && lhs.y == rhs.y && lhs.width == rhs.width &&
         lhs.height == rhs.height;
}
bool operator==(const rect32s& lhs, const rect32s& rhs) {
  return lhs.rect32_list == rhs.rect32_list;
}
bool operator==(const person& lhs, const person& rhs) {
  return lhs.id == rhs.id && lhs.name == rhs.name && lhs.age == rhs.age &&
         lhs.salary == rhs.salary;
}
bool operator==(const persons& lhs, const persons& rhs) {
  return lhs.person_list == rhs.person_list;
}
bool operator==(const Monster& lhs, const Monster& rhs) {
  bool ok = check_unique_ptr(lhs.pos, rhs.pos);
  if (!ok) {
    return false;
  }
  ok = check_unique_ptr(lhs.equipped, rhs.equipped);
  if (!ok) {
    return false;
  }
  return lhs.mana == rhs.mana && lhs.hp == rhs.hp && lhs.name == rhs.name &&
         lhs.inventory == rhs.inventory && lhs.color == rhs.color &&
         lhs.weapons == rhs.weapons && lhs.path == rhs.path;
};
bool operator==(const Monsters& lhs, const Monsters& rhs) {
  return lhs.monsters == rhs.monsters;
}
}  // namespace struct_pb_sample
bool verify(const struct_pb_sample::Weapon& a,
            const struct_pb_sample::Weapon& b) {
  assert(a.name == b.name);
  assert(a.damage == b.damage);
  return true;
}
bool verify(const struct_pb_sample::Monster& a,
            const struct_pb_sample::Monster& b) {
  assert(a.pos && b.pos);
  assert(*a.pos == *b.pos);
  assert(a.mana == b.mana);
  assert(a.hp == b.hp);
  assert(a.name == b.name);
  assert(a.inventory == b.inventory);
  assert(a.color == b.color);
  assert(a.weapons.size() == b.weapons.size());
  for (int i = 0; i < a.weapons.size(); ++i) {
    auto ok = verify(a.weapons[i], b.weapons[i]);
    if (!ok) {
      return ok;
    }
  }
  assert(a.weapons == b.weapons);
  assert(a.equipped && b.equipped);
  assert(*a.equipped == *b.equipped);
  assert(a.path == b.path);
  return true;
}
bool verify(const struct_pb_sample::Monsters& a,
            const struct_pb_sample::Monsters& b) {
  assert(a.monsters.size() == b.monsters.size());
  for (int i = 0; i < a.monsters.size(); ++i) {
    auto ok = verify(a.monsters[i], b.monsters[i]);
    if (!ok) {
      return ok;
    }
  }
  return true;
}
template <typename T>
T copy(const T& t) {
  return t;
}
template <>
struct_pb_sample::Monster copy(const struct_pb_sample::Monster& t) {
  struct_pb_sample::Monster m;
  m.pos = std::make_unique<struct_pb_sample::Vec3>();
  *m.pos = *t.pos;
  m.mana = t.mana;
  m.hp = t.hp;
  m.name = t.name;
  m.inventory = t.inventory;
  m.color = t.color;
  m.weapons = t.weapons;
  m.equipped = std::make_unique<struct_pb_sample::Weapon>();
  *m.equipped = *t.equipped;
  m.path = t.path;
  return m;
}
template <>
struct_pb_sample::Monsters copy(const struct_pb_sample::Monsters& t) {
  struct_pb_sample::Monsters m;
  m.monsters.reserve(t.monsters.size());
  for (const auto& e : t.monsters) {
    m.monsters.push_back(std::move(copy(e)));
  }
  return m;
}

struct struct_pb_sample_t : public base_sample {
  std::string name() const override { return "struct_pb"; }

  void create_samples() override {
    rects_ = struct_pb_sample::create_rects(OBJECT_COUNT);
    persons_ = struct_pb_sample::create_persons(OBJECT_COUNT);
    monsters_ = struct_pb_sample::create_monsters(OBJECT_COUNT);
  }

  void do_serialization(int run_idx) override {
    serialize(SampleType::RECT, rects_.rect32_list[0]);
    serialize(SampleType::RECTS, rects_);
    serialize(SampleType::PERSON, persons_.person_list[0]);
    serialize(SampleType::PERSONS, persons_);
    serialize(SampleType::MONSTER, monsters_.monsters[0]);
    serialize(SampleType::MONSTERS, monsters_);
  }

  void do_deserialization(int run_idx) override {
    deserialize(SampleType::RECT, rects_.rect32_list[0]);
    deserialize(SampleType::RECTS, rects_);
    deserialize(SampleType::PERSON, persons_.person_list[0]);
    deserialize(SampleType::PERSONS, persons_);
    deserialize(SampleType::MONSTER, monsters_.monsters[0]);
    deserialize(SampleType::MONSTERS, monsters_);
  }

 private:
  void serialize(SampleType sample_type, auto& sample) {
    {
      auto sz = struct_pb::internal::get_needed_size(sample);
      buffer_.resize(sz);
      struct_pb::internal::serialize_to(buffer_.data(), buffer_.size(), sample);
      buffer_.clear();
      buffer_.resize(sz);

      uint64_t ns = 0;
      std::string bench_name =
          name() + " serialize " + get_bench_name(sample_type);

      {
        ScopedTimer timer(bench_name.data(), ns);
        struct_pb::internal::serialize_to(buffer_.data(), buffer_.size(),
                                          sample);
      }
      ser_time_elapsed_map_.emplace(sample_type, buffer_.size());
    }
    buf_size_map_.emplace(sample_type, buffer_.size());
  }

  void deserialize(SampleType sample_type, auto& sample) {
    using T = std::remove_cvref_t<decltype(sample)>;
    buffer_.clear();
    auto sz = struct_pb::internal::get_needed_size(sample);
    buffer_.resize(sz);
    struct_pb::internal::serialize_to(buffer_.data(), buffer_.size(), sample);

    if constexpr (std::same_as<T, struct_pb_sample::Monsters>) {
      sample.monsters.clear();
      sample.monsters.reserve(OBJECT_COUNT);
    }
    else if constexpr (std::same_as<T, struct_pb_sample::rect32s>) {
      sample.rect32_list.clear();
      sample.rect32_list.reserve(OBJECT_COUNT);
    }
    else if constexpr (std::same_as<T, struct_pb_sample::persons>) {
      sample.person_list.clear();
      sample.person_list.reserve(OBJECT_COUNT);
    }

    uint64_t ns = 0;
    std::string bench_name =
        name() + " deserialize " + get_bench_name(sample_type);

    {
      ScopedTimer timer(bench_name.data(), ns);
      struct_pb::internal::deserialize_to(sample, buffer_.data(),
                                          buffer_.size());
    }
    deser_time_elapsed_map_.emplace(sample_type, ns);
  }

  struct_pb_sample::rect32s rects_;
  struct_pb_sample::persons persons_;
  struct_pb_sample::Monsters monsters_;
  std::string buffer_;
};