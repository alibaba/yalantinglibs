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
#include <atomic>
#include <msgpack.hpp>
#include <numeric>
#include <thread>
#include <valarray>

#include "benchmark.pb.h"
#include "no_op.h"
#include "struct_pack/struct_pack.hpp"

class ScopedTimer {
 public:
  ScopedTimer(const char *name)
      : m_name(name), m_beg(std::chrono::high_resolution_clock::now()) {}
  ScopedTimer(const char *name, uint64_t &ns) : ScopedTimer(name) {
    m_ns = &ns;
  }
  ~ScopedTimer() {
    auto end = std::chrono::high_resolution_clock::now();
    auto dur =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - m_beg);
    if (m_ns)
      *m_ns = dur.count();
    else
      std::cout << m_name << " : " << dur.count() << " ns\n";
  }

 private:
  const char *m_name;
  std::chrono::time_point<std::chrono::high_resolution_clock> m_beg;
  uint64_t *m_ns = nullptr;
};

struct person {
  int32_t id;
  std::string name;
  int age;
  double salary;
  MSGPACK_DEFINE(id, name, age, salary);
};

template <typename T>
T get_max() {
  if constexpr (std::is_same_v<T, int8_t>) {
    return 127;
  }
  else if constexpr (std::is_same_v<T, uint8_t>) {
    return 255;
  }
  else if constexpr (std::is_same_v<T, int16_t>) {
    return 32767;
  }
  else if constexpr (std::is_same_v<T, uint16_t>) {
    return 65535;
  }
  else if constexpr (std::is_same_v<T, int32_t>) {
    return INT_MAX - 1;
  }
  else if constexpr (std::is_same_v<T, uint32_t>) {
    return UINT_MAX - 1;
  }
  else if constexpr (std::is_same_v<T, int64_t>) {
    return LLONG_MAX - 1;
  }
  else if constexpr (std::is_same_v<T, uint64_t>) {
    return ULLONG_MAX - 1;
  }

  return {};
}

template <typename T>
struct rect {
  T x = get_max<T>();
  T y = get_max<T>();
  T width = get_max<T>();
  T height = get_max<T>();
  MSGPACK_DEFINE(x, y, width, height);
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
  MSGPACK_DEFINE(x, y, z);
};

struct Weapon {
  std::string name;
  int16_t damage;

  bool operator==(const Weapon &rhs) const {
    return name == rhs.name && damage == rhs.damage;
  };
  MSGPACK_DEFINE(name, damage);
};

struct Monster {
  Vec3 pos;
  int16_t mana;
  int16_t hp;
  std::string name;
  std::vector<uint8_t> inventory;
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
  MSGPACK_DEFINE(pos, mana, hp, name, inventory, (int &)color, weapons,
                 equipped, path);
};

static constexpr int OBJECT_COUNT = 20;
static constexpr int SAMPLES_COUNT = 100000;

template <typename T>
inline uint64_t get_avg(const T &v) {
  uint64_t sum = std::accumulate(v.begin(), v.end(), uint64_t(0));
  return sum / v.size();
}

struct tbuffer : public std::vector<char> {
  void write(const char *src, size_t sz) {
    this->resize(this->size() + sz);
    auto pos = this->data() + this->size() - sz;
    std::memcpy(pos, src, sz);
  }
};

std::vector<char> buffer1;
tbuffer buffer2;
std::string buffer3;

template <typename T, typename PB>
void bench(T &t, PB &p, std::string tag) {
  buffer1.clear();
  buffer2.clear();
  buffer3.clear();

  std::cout << "------- start benchmark " << tag << " -------\n";

  buffer1.reserve(struct_pack::get_needed_size(t));
  msgpack::pack(buffer2, t);
  buffer2.reserve(buffer2.size() * SAMPLES_COUNT);
  auto pb_sz = p.SerializeAsString().size();
  buffer3.reserve(pb_sz * SAMPLES_COUNT);

  std::array<std::array<uint64_t, 10>, 6> arr;

  for (int i = 0; i < 10; ++i) {
    buffer1.clear();
    buffer2.clear();
    buffer3.clear();
    {
      ScopedTimer timer("serialize structpack", arr[0][i]);
      for (int j = 0; j < SAMPLES_COUNT; j++) {
        struct_pack::serialize_to(buffer1, t);
      }
      no_op();
    }
    {
      ScopedTimer timer("serialize msgpack   ", arr[1][i]);
      for (int j = 0; j < SAMPLES_COUNT; j++) {
        msgpack::pack(buffer2, t);
      }
      no_op();
    }
    {
      ScopedTimer timer("serialize protobuf", arr[2][i]);
      for (int j = 0; j < SAMPLES_COUNT; j++) {
        buffer3.resize(buffer3.size() + pb_sz);
        p.SerializeToArray(buffer3.data() + buffer3.size() - pb_sz, pb_sz);
      }
      no_op();
    }
  }

  msgpack::unpacked unpacked;
  for (int i = 0; i < 10; ++i) {
    {
      ScopedTimer timer("deserialize structpack", arr[3][i]);
      std::size_t len = 0;
      for (size_t j = 0; j < SAMPLES_COUNT; j++) {
        auto ec = struct_pack::deserialize_to_with_offset(t, buffer1, len);
        if (ec != std::errc{}) [[unlikely]] {
          exit(1);
        }
        no_op();
      }
    }
    {
      ScopedTimer timer("deserialize msgpack   ", arr[4][i]);
      for (size_t pos = 0, j = 0; j < SAMPLES_COUNT; j++) {
        msgpack::unpack(unpacked, buffer2.data(), buffer2.size(), pos);
        t = unpacked.get().as<T>();
        no_op();
      }
    }
    {
      ScopedTimer timer("deserialize protobuf", arr[5][i]);
      for (size_t pos = 0, j = 0; j < SAMPLES_COUNT; j++) {
        if (!(p.ParseFromArray(buffer3.data() + pos, pb_sz))) [[unlikely]] {
          exit(1);
        }
        pos += pb_sz;
        no_op();
      }
    }
  }

  std::cout << tag << " "
            << "struct_pack serialize average: " << get_avg(arr[0])
            << ", deserialize average:  " << get_avg(arr[3])
            << ", buf size: " << buffer1.size() / SAMPLES_COUNT << "\n";
  std::cout << tag << " "
            << "msgpack serialize average:     " << get_avg(arr[1])
            << ", deserialize average: " << get_avg(arr[4])
            << ", buf size: " << buffer2.size() / SAMPLES_COUNT << "\n";
  std::cout << tag << " "
            << "protobuf serialize average:    " << get_avg(arr[2])
            << ", deserialize average: " << get_avg(arr[5])
            << ", buf size: " << buffer3.size() / SAMPLES_COUNT << "\n";
  std::cout << tag << " "
            << "struct_pack serialize is   "
            << (double)get_avg(arr[1]) / get_avg(arr[0])
            << " times faster than msgpack\n";
  std::cout << tag << " "
            << "struct_pack serialize is   "
            << (double)get_avg(arr[2]) / get_avg(arr[0])
            << " times faster than protobuf\n";
  std::cout << tag << " "
            << "struct_pack deserialize is "
            << (double)get_avg(arr[4]) / get_avg(arr[3])
            << " times faster than msgpack\n";
  std::cout << tag << " "
            << "struct_pack deserialize is "
            << (double)get_avg(arr[5]) / get_avg(arr[3])
            << " times faster than protobuf\n";
  std::cout << "------- end benchmark   " << tag << " -------\n\n";
}

auto create_rects(size_t object_count) {
  rect<int32_t> rc{65536, 65536, 65536, 65536};
  std::vector<rect<int32_t>> v{};
  for (int i = 0; i < object_count; i++) {
    v.push_back(rc);
  }
  return v;
}

auto create_rects_pb(size_t object_count) {
  mygame::rect32s rcs{};
  for (int i = 0; i < object_count; i++) {
    mygame::rect32 *rc = rcs.add_rect32_list();
    rc->set_height(65536);
    rc->set_width(65536);
    rc->set_x(65536);
    rc->set_y(65536);
  }
  return rcs;
}

auto create_persons(size_t object_count) {
  std::vector<person> v{};
  person p{65536, "tom", 65536, 65536.42};
  for (int i = 0; i < object_count; i++) {
    v.push_back(p);
  }
  return v;
}

mygame::persons create_persons_pb(size_t object_count) {
  mygame::persons ps;
  for (int i = 0; i < object_count; i++) {
    auto *p = ps.add_person_list();
    p->set_age(65536);
    p->set_name("tom");
    p->set_id(65536);
    p->set_salary(65536.42);
  }
  return ps;
}

std::vector<Monster> create_monsters(size_t object_count) {
  std::vector<Monster> v{};
  Monster m = {
      .pos = {1, 2, 3},
      .mana = 16,
      .hp = 24,
      .name = "it is a test",
      .inventory = {1, 2, 3, 4},
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
      .inventory = {24, 25, 25, 24},
      .color = Color::Red,
      .weapons = {{"gun", 421}, {"mission", 561}},
      .equipped = {"air craft", 671},
      .path = {{71, 82, 93}},
  };

  for (int i = 0; i < object_count / 2; i++) {
    v.push_back(m);
    v.push_back(m1);
  }

  return v;
}

mygame::Monsters create_monsters_pb(size_t object_count) {
  mygame::Monsters Monsters;
  for (int i = 0; i < object_count / 2; i++) {
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
      w1->set_name("mission");
      w1->set_damage(56);
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
      w1->set_name("mission");
      w1->set_damage(561);
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

auto v = create_rects(OBJECT_COUNT);
auto pbs = create_rects_pb(OBJECT_COUNT);
auto pb = *(pbs.rect32_list().begin());

auto v1 = create_persons(OBJECT_COUNT);
auto pbs1 = create_persons_pb(OBJECT_COUNT);
auto pb1 = *(pbs1.person_list().begin());

auto v2 = create_monsters(OBJECT_COUNT);
auto pbs2 = create_monsters_pb(OBJECT_COUNT);
auto pb2 = *(pbs2.monsters().begin());

int main() {
  {
    bench(v[0], pb, "1 rect");
    bench(v, pbs, "20 rect");
  }

  {
    bench(v1[0], pb1, "1 person");
    bench(v1, pbs1, "20 person");
  }

  {
    bench(v2[0], pb2, "1 monster");
    bench(v2, pbs2, "20 monster");
  }
}
