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

#ifdef HAVE_MSGPACK
#include <msgpack.hpp>
#endif
#include <atomic>
#include <numeric>
#include <thread>
#include <valarray>
struct person {
  int32_t id;
  std::string name;
  int age;
  double salary;
#ifdef HAVE_MSGPACK
  MSGPACK_DEFINE(id, name, age, salary);
#endif
};

template <typename T>
struct rect {
  T x;
  T y;
  T width;
  T height;
#ifdef HAVE_MSGPACK
  MSGPACK_DEFINE(x, y, width, height);
#endif
};

enum Color : uint8_t { Red, Green, Blue };

struct Vec3 {
  float x;
  float y;
  float z;

  bool operator==(const Vec3 &rhs) const {
    std::valarray<float> lh({x, y, z});
    std::valarray<float> rh({rhs.x, rhs.y, rhs.z});
    return (std::abs(lh - rh) < 0.05f).min();
  };
#ifdef HAVE_MSGPACK
  MSGPACK_DEFINE(x, y, z);
#endif
};

struct Weapon {
  std::string name;
  int16_t damage;

  bool operator==(const Weapon &rhs) const {
    return name == rhs.name && damage == rhs.damage;
  };
#ifdef HAVE_MSGPACK
  MSGPACK_DEFINE(name, damage);
#endif
};

struct Monster {
  Vec3 pos;
  int16_t mana;
  int16_t hp;
  std::string name;
  std::string inventory;
  Color color;
  std::vector<Weapon> weapons;
  Weapon equipped;
  std::vector<Vec3> path;

  bool operator==(const Monster &rhs) const {
    return pos == rhs.pos && mana == rhs.mana && hp == rhs.hp &&
           name == rhs.name && inventory == rhs.inventory &&
           color == rhs.color && weapons == rhs.weapons &&
           equipped == rhs.equipped && path == rhs.path;
  };
#ifdef HAVE_MSGPACK
  MSGPACK_DEFINE(pos, mana, hp, name, inventory, (int &)color, weapons,
                 equipped, path);
#endif
};

inline auto create_rects(size_t object_count) {
  rect<int32_t> rc{65536, 65536, 65536, 65536};
  std::vector<rect<int32_t>> v{};
  for (int i = 0; i < object_count; i++) {
    v.push_back(rc);
  }
  return v;
}

inline auto create_persons(size_t object_count) {
  std::vector<person> v{};
  person p{65536, "tom", 65536, 65536.42};
  for (int i = 0; i < object_count; i++) {
    v.push_back(p);
  }
  return v;
}

inline std::vector<Monster> create_monsters(size_t object_count) {
  std::vector<Monster> v{};
  Monster m = {
      .pos = {1, 2, 3},
      .mana = 16,
      .hp = 24,
      .name = "it is a test",
      .inventory = "\1\2\3\4",
      .color = Color::Red,
      .weapons = {{"gun", 42}, {"mission", 56}},
      .equipped = {"air craft", 67},
      .path = {{7, 8, 9}},
  };

  Monster m1 = {
      .pos = {11, 22, 33},
      .mana = 161,
      .hp = 241,
      .name = "it is a test, ok",
      .inventory = "\24\25\26\24",
      .color = Color::Red,
      .weapons = {{"gun", 421}, {"mission", 561}},
      .equipped = {"air craft", 671},
      .path = {{71, 82, 93}},
  };

  for (int i = 0; i < object_count / 2; i++) {
    v.push_back(m);
    v.push_back(m1);
  }

  if (object_count % 2 == 1) {
    v.push_back(m);
  }

  return v;
}
