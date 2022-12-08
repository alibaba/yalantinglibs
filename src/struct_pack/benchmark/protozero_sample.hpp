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
#include "data_def.hpp"
#include "protozero/pbf_reader.hpp"
#include "protozero/pbf_writer.hpp"

namespace protozero_sample {
using namespace protozero;
void serialize(const Vec3& vec3, pbf_writer& writer) {
  writer.add_float(1, vec3.x);
  writer.add_float(2, vec3.y);
  writer.add_float(3, vec3.z);
}
void serialize(const Weapon& weapon, pbf_writer& writer) {
  writer.add_string(1, weapon.name);
  writer.add_int32(2, weapon.damage);
}
void serialize(const Monster& monster, pbf_writer& writer) {
  {
    pbf_writer pbf_sub{writer, 1};
    serialize(monster.pos, pbf_sub);
  }
  writer.add_int32(2, monster.mana);
  writer.add_int32(3, monster.hp);
  writer.add_string(4, monster.name);
  writer.add_bytes(5, monster.inventory);
  writer.add_enum(6, monster.color);
  {
    for (const auto& weapon : monster.weapons) {
      pbf_writer pbf_sub{writer, 7};
      serialize(weapon, pbf_sub);
    }
  }
  {
    pbf_writer pbf_sub{writer, 8};
    serialize(monster.equipped, pbf_sub);
  }
  for (const auto& v : monster.path) {
    pbf_writer pbf_sub{writer, 9};
    serialize(v, pbf_sub);
  }
}
template <typename T>
void serialize(const std::vector<T>& ms, pbf_writer& writer) {
  for (const auto& m : ms) {
    pbf_writer pbf_sub{writer, 1};
    serialize(m, pbf_sub);
  }
}
void deserialize(Vec3& vec3, pbf_reader& reader) {
  while (reader.next()) {
    switch (reader.tag()) {
      case 1:
        vec3.x = reader.get_float();
        break;
      case 2:
        vec3.y = reader.get_float();
        break;
      case 3:
        vec3.z = reader.get_float();
        break;
      default:
        reader.skip();
    }
  }
}
void deserialize(Weapon& weapon, pbf_reader& reader) {
  while (reader.next()) {
    switch (reader.tag()) {
      case 1:
        weapon.name = reader.get_string();
        break;
      case 2:
        weapon.damage = reader.get_int32();
        break;
      default:
        reader.skip();
    }
  }
}
void deserialize(Monster& monster, pbf_reader& reader) {
  while (reader.next()) {
    switch (reader.tag()) {
      case 1: {
        auto reader_sub = reader.get_message();
        deserialize(monster.pos, reader_sub);
        break;
      }
      case 2:
        monster.mana = reader.get_int32();
        break;
      case 3:
        monster.hp = reader.get_int32();
        break;
      case 4:
        monster.name = reader.get_string();
        break;
      case 5: {
        monster.inventory = reader.get_string();
        break;
      }
      case 6:
        monster.color = static_cast<Color>(reader.get_enum());
        break;
      case 7: {
        auto reader_sub = reader.get_message();
        Weapon w{};
        deserialize(w, reader_sub);
        monster.weapons.push_back(w);
        break;
      }
      case 8: {
        auto reader_sub = reader.get_message();
        deserialize(monster.equipped, reader_sub);
        break;
      }
      case 9: {
        auto reader_sub = reader.get_message();
        Vec3 v{};
        deserialize(v, reader_sub);
        monster.path.push_back(v);
        break;
      }
    }
  }
}
template <typename T>
void deserialize(std::vector<T>& ms, pbf_reader& reader) {
  while (reader.next()) {
    switch (reader.tag()) {
      case 1: {
        auto reader_sub = reader.get_message();
        T m{};
        deserialize(m, reader_sub);
        ms.push_back(m);
        break;
      }
      default: {
        reader.skip();
      }
    }
  }
}
}  // namespace protozero_sample

template <typename Data, typename Buffer>
struct Sample<SampleName::PROTOZERO, Data, Buffer> : public SampleBase {
  Sample(Data data) : data_(std::move(data)) {
    protozero::pbf_writer writer{buffer_};
    protozero_sample::serialize(data_, writer);
    size_ = buffer_.size();
  }
  void clear_data() override {
    if constexpr (std::same_as<Data, std::vector<Monster>>) {
      data_.clear();
    }
  }
  void clear_buffer() override { buffer_.clear(); }
  void reserve_buffer() override {
    buffer_.resize(buffer_.size() * SAMPLES_COUNT);
  }
  size_t buffer_size() const override { return buffer_.size(); }
  std::string name() const override { return "protozero"; }
  void do_serialization(int run_idx) override {
    ScopedTimer timer(("serialize " + name()).c_str(),
                      serialize_cost_[run_idx]);
    for (int i = 0; i < SAMPLES_COUNT; ++i) {
      protozero::pbf_writer writer{buffer_};
      protozero_sample::serialize(data_, writer);
    }
  }
  void do_deserialization(int run_idx) override {
    ScopedTimer timer(("deserialize " + name()).c_str(),
                      deserialize_cost_[run_idx]);
    std::size_t pos = 0;
    for (int i = 0; i < SAMPLES_COUNT; ++i) {
      protozero::pbf_reader reader{buffer_.data() + pos, size_};
      protozero_sample::deserialize(data_, reader);
      pos += size_;
    }
  }

 private:
  Data data_;
  Buffer buffer_;
  std::size_t size_;
};
namespace protozero_sample {
template <SampleType sample_type>
auto create_sample() {
  using Buffer = std::string;
  if constexpr (sample_type == SampleType::RECT) {
    using Data = rect<int32_t>;
    auto rects = create_rects(OBJECT_COUNT);
    return Sample<SampleName::PROTOZERO, Data, Buffer>(rects[0]);
  }
  else if constexpr (sample_type == SampleType::RECTS) {
    using Data = std::vector<rect<int32_t>>;
    auto rects = create_rects(OBJECT_COUNT);
    return Sample<SampleName::PROTOZERO, Data, Buffer>(rects);
  }
  else if constexpr (sample_type == SampleType::PERSON) {
    using Data = person;
    auto persons = create_persons(OBJECT_COUNT);
    auto person = persons[0];
    return Sample<SampleName::PROTOZERO, Data, Buffer>(person);
  }
  else if constexpr (sample_type == SampleType::PERSONS) {
    using Data = std::vector<person>;
    auto persons = create_persons(OBJECT_COUNT);
    return Sample<SampleName::PROTOZERO, Data, Buffer>(persons);
  }
  else if constexpr (sample_type == SampleType::MONSTER) {
    using Data = Monster;
    auto monsters = create_monsters(OBJECT_COUNT);
    auto monster = monsters[0];
    return Sample<SampleName::PROTOZERO, Data, Buffer>(monster);
  }
  else if constexpr (sample_type == SampleType::MONSTERS) {
    using Data = std::vector<Monster>;
    auto monsters = create_monsters(OBJECT_COUNT);
    return Sample<SampleName::PROTOZERO, Data, Buffer>(monsters);
  }
  else {
    return sample_type;
  }
}
}  // namespace protozero_sample