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

#include "ScopedTimer.hpp"
#include "benchmark.pb.h"
#include "config.hpp"
#include "sample.hpp"
namespace protobuf_sample {
auto create_rects(size_t object_count) {
  mygame::rect32s rcs{};
  for (std::size_t i = 0; i < object_count; i++) {
    mygame::rect32 *rc = rcs.add_rect32_list();
    rc->set_height(65536);
    rc->set_width(65536);
    rc->set_x(65536);
    rc->set_y(65536);
  }
  return rcs;
}

mygame::persons create_persons(size_t object_count) {
  mygame::persons ps;
  for (std::size_t i = 0; i < object_count; i++) {
    auto *p = ps.add_person_list();
    p->set_age(65536);
    p->set_name("tom");
    p->set_id(65536);
    p->set_salary(65536.42);
  }
  return ps;
}

mygame::Monsters create_monsters(size_t object_count) {
  mygame::Monsters Monsters;
  for (std::size_t i = 0; i < object_count / 2; i++) {
    {
      auto m = Monsters.add_monsters();
      auto vec = new mygame::Vec3;
      vec->set_x(1);
      vec->set_y(2);
      vec->set_z(3);
      m->set_allocated_pos(vec);
      m->set_mana(16);
      m->set_hp(24);
      m->set_name("it is a test");
      m->set_inventory("\1\2\3\4");
      m->set_color(::mygame::Monster_Color::Monster_Color_Red);
      auto w1 = m->add_weapons();
      w1->set_name("gun");
      w1->set_damage(42);
      auto w2 = m->add_weapons();
      w2->set_name("mission");
      w2->set_damage(56);
      auto w3 = new mygame::Weapon;
      w3->set_name("air craft");
      w3->set_damage(67);
      m->set_allocated_equipped(w3);
      auto p1 = m->add_path();
      p1->set_x(7);
      p1->set_y(8);
      p1->set_z(9);
    }
    {
      auto m = Monsters.add_monsters();
      auto vec = new mygame::Vec3;
      vec->set_x(11);
      vec->set_y(22);
      vec->set_z(33);
      m->set_allocated_pos(vec);
      m->set_mana(161);
      m->set_hp(241);
      m->set_name("it is a test, ok");
      m->set_inventory("\24\25\26\24");
      m->set_color(::mygame::Monster_Color::Monster_Color_Red);
      auto w1 = m->add_weapons();
      w1->set_name("gun");
      w1->set_damage(421);
      auto w2 = m->add_weapons();
      w2->set_name("mission");
      w2->set_damage(561);
      auto w3 = new mygame::Weapon;
      w3->set_name("air craft");
      w3->set_damage(671);
      m->set_allocated_equipped(w3);
      auto p1 = m->add_path();
      p1->set_x(71);
      p1->set_y(82);
      p1->set_z(93);
    }
  }
  return Monsters;
}
}  // namespace protobuf_sample
template <typename Data, typename Buffer>
struct Sample<SampleName::PROTOBUF, Data, Buffer> : public SampleBase {
  Sample(Data data) : data_(std::move(data)) {
    pb_size_ = data_.SerializeAsString().size();
  }
  void clear_data() override {
    if constexpr (std::same_as<Data, mygame::Monsters>) {
      data_.clear_monsters();
    }
    else if constexpr (std::same_as<Data, mygame::Monster>) {
      data_.clear_pos();
      data_.set_mana(0);
      data_.set_hp(0);
      data_.clear_name();
      data_.clear_inventory();
      data_.clear_color();
      data_.clear_weapons();
      data_.clear_equipped();
      data_.clear_path();
    }
  }
  void clear_buffer() override { buffer_.clear(); }
  void reserve_buffer() override { buffer_.reserve(pb_size_ * SAMPLES_COUNT); }
  size_t buffer_size() const override { return buffer_.size(); }
  std::string name() const override { return "protobuf"; }
  void do_serialization(int run_idx = 0) override {
    ScopedTimer timer(("serialize " + name()).c_str(),
                      serialize_cost_[run_idx]);
    for (std::size_t i = 0; i < SAMPLES_COUNT; ++i) {
      buffer_.resize(buffer_.size() + pb_size_);
      data_.SerializeToArray(buffer_.data() + buffer_.size() - pb_size_,
                             pb_size_);
    }
  }
  void do_deserialization(int run_idx) override {
    ScopedTimer timer(("deserialize " + name()).c_str(),
                      deserialize_cost_[run_idx]);
    std::size_t pos = 0;
    for (std::size_t i = 0; i < SAMPLES_COUNT; ++i) {
      if (!data_.ParseFromArray(buffer_.data() + pos, pb_size_)) [[unlikely]] {
        std::exit(1);
      }
      pos += pb_size_;
      clear_data();
    }
  }

 private:
  Data data_;
  Buffer buffer_;
  std::size_t pb_size_;
};
namespace protobuf_sample {
template <SampleType sample_type>
auto create_sample() {
  using Buffer = std::string;
  if constexpr (sample_type == SampleType::RECT) {
    using Data = mygame::rect32;
    auto pbs = create_rects(OBJECT_COUNT);
    auto pb = *(pbs.rect32_list().begin());
    return Sample<SampleName::PROTOBUF, Data, Buffer>(pb);
  }
  else if constexpr (sample_type == SampleType::RECTS) {
    using Data = mygame::rect32s;
    auto pbs = create_rects(OBJECT_COUNT);
    return Sample<SampleName::PROTOBUF, Data, Buffer>(pbs);
  }
  else if constexpr (sample_type == SampleType::PERSON) {
    using Data = mygame::person;
    auto pbs = create_persons(OBJECT_COUNT);
    auto pb = *(pbs.person_list().begin());
    return Sample<SampleName::PROTOBUF, Data, Buffer>(pb);
  }
  else if constexpr (sample_type == SampleType::PERSONS) {
    using Data = mygame::persons;
    auto pbs = create_persons(OBJECT_COUNT);
    return Sample<SampleName::PROTOBUF, Data, Buffer>(pbs);
  }
  else if constexpr (sample_type == SampleType::MONSTER) {
    using Data = mygame::Monster;
    auto monsters = create_monsters(OBJECT_COUNT);
    auto monster = *(monsters.monsters().begin());
    return Sample<SampleName::PROTOBUF, Data, Buffer>(monster);
  }
  else if constexpr (sample_type == SampleType::MONSTERS) {
    using Data = mygame::Monsters;
    auto monsters = create_monsters(OBJECT_COUNT);
    return Sample<SampleName::PROTOBUF, Data, Buffer>(monsters);
  }
  else {
    return sample_type;
  }
}
}  // namespace protobuf_sample
