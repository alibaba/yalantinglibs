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
#include "protopuf/message.h"
namespace protopuf_sample {
using namespace pp;

using Vec3 =
    message<float_field<"x", 1>, float_field<"y", 2>, float_field<"z", 3> >;
using Weapon = message<string_field<"name", 1>, int32_field<"damage", 2> >;
enum class Color {
  Red = 0,
  Green = 1,
  Blue = 2,
};
using Monster =
    message<message_field<"pos", 1, Vec3>, int32_field<"mana", 2>,
            int32_field<"hp", 3>, string_field<"name", 4>,
            bytes_field<"inventory", 5>, enum_field<"color", 6, Color>,
            message_field<"weapons", 7, Weapon, repeated>,
            message_field<"equipped", 8, Weapon>,
            message_field<"path", 9, Vec3, repeated> >;
using Monsters = message<message_field<"monsters", 1, Monster, repeated> >;
using rect32 = message<int32_field<"x", 1>, int32_field<"y", 2>,
                       int32_field<"width", 3>, int32_field<"height", 4> >;
using rect32s = message<message_field<"rect32_list", 1, rect32, repeated> >;
using person = message<int32_field<"id", 1>, string_field<"name", 2>,
                       int32_field<"age", 3>, double_field<"salary", 4> >;
using persons = message<message_field<"person_list", 1, person, repeated> >;

auto create_rects(std::size_t object_count) {
  rect32s rcs;
  for (std::size_t i = 0; i < object_count; ++i) {
    rect32 rc{65536, 65536, 65536, 65536};
    rcs["rect32_list"_f].push_back(rc);
  }
  return rcs;
}
auto create_persons(std::size_t object_count) {
  persons ps;
  for (std::size_t i = 0; i < object_count; ++i) {
    person p{65536, "tom", 65536, 65536.42};
    ps["person_list"_f].push_back(p);
  }
  return ps;
}
auto create_monsters(std::size_t object_count) {
  Monsters monsters;
  for (std::size_t i = 0; i < object_count / 2; ++i) {
    {
      Monster m;
      m["pos"_f] = Vec3{1, 2, 3};
      m["mana"_f] = 16;
      m["hp"_f] = 24;
      m["name"_f] = "it is a test";
      m["inventory"_f] = std::vector<unsigned char>{'\1', '\2', '\3', '\4'};
      // LLVM crash
      // static_assert(std::same_as<decltype(m["inventory"_f]), int32_t>);
      m["color"_f] = Color::Red;
      m["weapons"_f].emplace_back("gun", 42);
      m["weapons"_f].emplace_back("mission", 56);
      m["equipped"_f] = Weapon{"air craft", 67};
      m["path"_f] = std::vector<Vec3>{Vec3{7, 8, 9}};
      monsters["monsters"_f].push_back(m);
    }
    {
      Monster m;
      m["pos"_f] = Vec3{11, 22, 33};
      m["mana"_f] = 161;
      m["hp"_f] = 241;
      m["name"_f] = "it is a test, ok";
      m["inventory"_f] = std::vector<unsigned char>{'\24', '\25', '\26', '\24'};
      m["color"_f] = Color::Red;
      m["weapons"_f].emplace_back("gun", 421);
      m["weapons"_f].emplace_back("mission", 561);
      m["equipped"_f] = Weapon{"air craft", 671};
      m["path"_f] = std::vector<Vec3>{Vec3{71, 82, 93}};
      monsters["monsters"_f].push_back(m);
    }
  }
  return monsters;
}
}  // namespace protopuf_sample

template <typename Data, typename Buffer>
struct Sample<SampleName::PROTOPUF, Data, Buffer> : public SampleBase {
  Sample(Data data) : data_(std::move(data)) {}
  void clear_data() override {
    using namespace pp;
    if constexpr (std::same_as<Data, protopuf_sample::Monsters>) {
      data_["monsters"_f].clear();
    }
  }
  void clear_buffer() override { buffer_ = Buffer{}; }
  void reserve_buffer() override {}
  size_t buffer_size() const override { return buffer_.size(); }
  std::string name() const override { return "protopuf"; }
  void do_serialization(int run_idx) override {
    ScopedTimer timer(("serialize " + name()).c_str(),
                      serialize_cost_[run_idx]);
    for (std::size_t i = 0; i < SAMPLES_COUNT; ++i) {
      pp::message_coder<Data>::encode(data_, buffer_);
    }
  }
  void do_deserialization(int run_idx) override {
    ScopedTimer timer(("deserialize " + name()).c_str(),
                      deserialize_cost_[run_idx]);
    for (std::size_t i = 0; i < SAMPLES_COUNT; ++i) {
      pp::message_coder<Data>::decode(buffer_);
    }
  }

 private:
  Data data_;
  Buffer buffer_;
};
namespace protopuf_sample {
template <SampleType sample_type>
auto create_sample() {
  using pp::operator""_f;
  using Buffer = std::array<std::byte, 3000 * OBJECT_COUNT>;
  if constexpr (sample_type == SampleType::RECT) {
    using Data = protopuf_sample::rect32;
    auto rects = protopuf_sample::create_rects(OBJECT_COUNT);
    auto rect = rects["rect32_list"_f][0];
    return Sample<SampleName::PROTOPUF, Data, Buffer>(rect);
  }
  else if constexpr (sample_type == SampleType::RECTS) {
    using Data = protopuf_sample::rect32s;
    auto rects = protopuf_sample::create_rects(OBJECT_COUNT);
    return Sample<SampleName::PROTOPUF, Data, Buffer>(rects);
  }
  else if constexpr (sample_type == SampleType::PERSON) {
    using Data = protopuf_sample::person;
    auto persons = protopuf_sample::create_persons(OBJECT_COUNT);
    auto person = persons["person_list"_f][0];
    return Sample<SampleName::PROTOPUF, Data, Buffer>(person);
  }
  else if constexpr (sample_type == SampleType::PERSONS) {
    using Data = protopuf_sample::persons;
    auto persons = protopuf_sample::create_persons(OBJECT_COUNT);
    return Sample<SampleName::PROTOPUF, Data, Buffer>(persons);
  }
  else if constexpr (sample_type == SampleType::MONSTER) {
    using Data = protopuf_sample::Monster;
    auto monsters = protopuf_sample::create_monsters(OBJECT_COUNT);
    auto monster = monsters["monsters"_f][0];
    return Sample<SampleName::PROTOPUF, Data, Buffer>(monster);
  }
  else if constexpr (sample_type == SampleType::MONSTERS) {
    using Data = protopuf_sample::Monsters;
    auto monsters = protopuf_sample::create_monsters(OBJECT_COUNT);
    return Sample<SampleName::PROTOPUF, Data, Buffer>(monsters);
  }
  else {
    return sample_type;
  }
}
}  // namespace protopuf_sample
