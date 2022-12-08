/*
 * Copyright (c) 2022, Alibaba Group Holding Limited;
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once
#include <optional>
#include <valarray>

#include "ScopedTimer.hpp"
#include "sample.hpp"
#include "struct_pack/struct_pack/pb.hpp"
namespace struct_pb_sample {
struct Vec3 {
  float x;
  float y;
  float z;

  bool operator==(const Vec3 &rhs) const {
    std::valarray<float> lh({x, y, z});
    std::valarray<float> rh({rhs.x, rhs.y, rhs.z});
    return (std::abs(lh - rh) < 0.05f).min();
  };
};
struct Weapon {
  std::string name;
  struct_pack::pb::varint32_t damage;
  bool operator==(const Weapon &rhs) const {
    return name == rhs.name && damage == rhs.damage;
  };
};
struct Monster {
  std::optional<Vec3> pos;
  struct_pack::pb::varint32_t mana;
  struct_pack::pb::varint32_t hp;
  std::string name;
  std::string inventory;
  enum class Color { Red, Green, Blue };
  Color color;
  std::vector<Weapon> weapons;
  std::optional<Weapon> equipped;
  std::vector<Vec3> path;
  bool operator==(const Monster &rhs) const {
    return pos == rhs.pos && mana == rhs.mana && hp == rhs.hp &&
           name == rhs.name && inventory == rhs.inventory &&
           color == rhs.color && weapons == rhs.weapons &&
           equipped == rhs.equipped && path == rhs.path;
  };
};
struct Monsters {
  std::vector<Monster> monsters;
  bool operator==(const Monsters &) const = default;
};
struct rect32 {
  struct_pack::pb::varint32_t x;
  struct_pack::pb::varint32_t y;
  struct_pack::pb::varint32_t width;
  struct_pack::pb::varint32_t height;
  bool operator==(const rect32 &rhs) const {
    return x == rhs.x && y == rhs.y && width == rhs.width &&
           height == rhs.height;
  }
};
struct rect32s {
  std::vector<rect32> rect32_list;
  bool operator==(const rect32s &rhs) const {
    return rect32_list == rhs.rect32_list;
  }
};
struct person {
  struct_pack::pb::varint32_t id;
  std::string name;
  struct_pack::pb::varint32_t age;
  double salary;
  bool operator==(const person &rhs) const {
    return id == rhs.id && name == rhs.name && age == rhs.age &&
           salary == rhs.salary;
  }
};
struct persons {
  std::vector<person> person_list;
  bool operator==(const persons &rhs) const {
    return person_list == rhs.person_list;
  }
};
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
      Monster m;
      m.pos = {1, 2, 3};
      m.mana = 16;
      m.hp = 24;
      m.name = "it is a test";
      m.inventory = "\1\2\3\4";
      m.color = Monster::Color::Red;
      m.weapons = {{"gun", 42}, {"mission", 56}};
      m.equipped = {"air craft", 67};
      m.path = {{7, 8, 9}};
      monsters.monsters.push_back(m);
    }
    {
      Monster m;
      m.pos = {11, 22, 33};
      m.mana = 161;
      m.hp = 241;
      m.name = "it is a test, ok";
      m.inventory = "\24\25\26\24";
      m.color = Monster::Color::Red;
      m.weapons = {{"gun", 421}, {"mission", 561}};
      m.equipped = {"air craft", 671};
      m.path = {{71, 82, 93}};
      monsters.monsters.push_back(m);
    }
  }
  return monsters;
}
}  // namespace struct_pb_sample
bool verify(const struct_pb_sample::Weapon &a,
            const struct_pb_sample::Weapon &b) {
  assert(a.name == b.name);
  assert(a.damage == b.damage);
  return true;
}
bool verify(const struct_pb_sample::Monster &a,
            const struct_pb_sample::Monster &b) {
  assert(a.pos == b.pos);
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
  assert(a.equipped == b.equipped);
  assert(a.path == b.path);
  return true;
}
bool verify(const struct_pb_sample::Monsters &a,
            const struct_pb_sample::Monsters &b) {
  assert(a.monsters.size() == b.monsters.size());
  for (int i = 0; i < a.monsters.size(); ++i) {
    auto ok = verify(a.monsters[i], b.monsters[i]);
    if (!ok) {
      return ok;
    }
  }
  return true;
}
template <typename Data, typename Buffer>
struct Sample<SampleName::STRUCT_PB, Data, Buffer> : public SampleBase {
  Sample(Data data) : data_(std::move(data)) {
    size_ = struct_pack::pb::get_needed_size(data_);
#ifndef NDEBUG
    data_old_ = data_;
#endif
  }
  void clear_data() override {
    if constexpr (std::same_as<Data, struct_pb_sample::Monsters>) {
      data_.monsters.clear();
    }
    else if constexpr (std::same_as<Data, struct_pb_sample::Monster>) {
      data_.pos.reset();
      data_.mana = 0;
      data_.hp = 0;
      data_.name.clear();
      data_.inventory.clear();
      data_.color = struct_pb_sample::Monster::Color::Red;
      data_.weapons.clear();
      data_.equipped.reset();
      data_.path.clear();
    }
  }
  void clear_buffer() override { buffer_.clear(); }
  void reserve_buffer() override {
    buffer_.reserve(struct_pack::pb::get_needed_size(data_) * SAMPLES_COUNT);
  }
  size_t buffer_size() const override { return buffer_.size(); }
  std::string name() const override { return "struct_pb"; }
  void do_serialization(int run_idx) override {
    ScopedTimer timer(("serialize " + name()).c_str(),
                      serialize_cost_[run_idx]);
    for (int i = 0; i < SAMPLES_COUNT; ++i) {
      struct_pack::pb::serialize_to(buffer_, data_);
    }
  }
  void do_deserialization(int run_idx) override {
    ScopedTimer timer(("deserialize " + name()).c_str(),
                      deserialize_cost_[run_idx]);
    std::size_t len = 0;
    std::size_t pos = 0;
    for (int i = 0; i < SAMPLES_COUNT; ++i) {
      auto ec = struct_pack::pb::deserialize_to<Data>(
          data_, buffer_.data() + pos, size_, len);
      if (ec != std::errc{}) [[unlikely]] {
        std::exit(1);
      }
      pos += size_;
      assert(verify(data_old_, data_));
      clear_data();
    }
  }

 private:
  Data data_;
  Buffer buffer_;
  std::size_t size_;
#ifndef NDEBUG
  Data data_old_;
#endif
};
namespace struct_pb_sample {
template <SampleType sample_type>
auto create_sample() {
  using Buffer = std::string;
  if constexpr (sample_type == SampleType::RECT) {
    using Data = struct_pb_sample::rect32;
    auto rects = create_rects(OBJECT_COUNT);
    auto rect = rects.rect32_list[0];
    return Sample<SampleName::STRUCT_PB, Data, Buffer>(rect);
  }
  else if constexpr (sample_type == SampleType::RECTS) {
    using Data = struct_pb_sample::rect32s;
    auto rects = create_rects(OBJECT_COUNT);
    return Sample<SampleName::STRUCT_PB, Data, Buffer>(rects);
  }
  else if constexpr (sample_type == SampleType::PERSON) {
    using Data = struct_pb_sample::person;
    auto persons = create_persons(OBJECT_COUNT);
    auto person = persons.person_list[0];
    return Sample<SampleName::STRUCT_PB, Data, Buffer>(person);
  }
  else if constexpr (sample_type == SampleType::PERSONS) {
    using Data = struct_pb_sample::persons;
    auto persons = create_persons(OBJECT_COUNT);
    return Sample<SampleName::STRUCT_PB, Data, Buffer>(persons);
  }
  else if constexpr (sample_type == SampleType::MONSTER) {
    using Data = struct_pb_sample::Monster;
    auto monsters = create_monsters(OBJECT_COUNT);
    auto monster = monsters.monsters[0];
    return Sample<SampleName::STRUCT_PB, Data, Buffer>(monster);
  }
  else if constexpr (sample_type == SampleType::MONSTERS) {
    using Data = struct_pb_sample::Monsters;
    auto monsters = create_monsters(OBJECT_COUNT);
    return Sample<SampleName::STRUCT_PB, Data, Buffer>(monsters);
  }
  else {
    return sample_type;
  }
}
}  // namespace struct_pb_sample
