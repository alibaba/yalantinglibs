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
#include "../benchmark/config.hpp"
#ifdef HAVE_PROTOBUF
#include "../benchmark/protobuf_sample.hpp"
#endif
#include "../benchmark/struct_pb_sample.hpp"
#include "doctest.h"
#include "helper.hpp"
using namespace doctest;
using namespace struct_pb::internal;

#ifdef HAVE_PROTOBUF
template <>
struct PB_equal<struct_pb_sample::rect32s, mygame::rect32s> {
  bool operator()(const struct_pb_sample::rect32s& t,
                  const mygame::rect32s& pb_t) const {
    return t.rect32_list == pb_t.rect32_list();
  }
};
template <>
struct PB_equal<struct_pb_sample::rect32, mygame::rect32> {
  bool operator()(const struct_pb_sample::rect32& lhs,
                  const mygame::rect32& rhs) const {
    return lhs.x == rhs.x() && lhs.y == rhs.y() && lhs.width == rhs.width() &&
           lhs.height == rhs.height();
  }
};
template <>
struct PB_equal<struct_pb_sample::Vec3, mygame::Vec3> {
  bool operator()(const struct_pb_sample::Vec3& lhs,
                  const mygame::Vec3& rhs) const {
    return lhs.x == rhs.x() && lhs.y == rhs.y() && lhs.z == rhs.z();
  }
};
template <>
struct PB_equal<struct_pb_sample::person, mygame::person> {
  bool operator()(const struct_pb_sample::person& lhs,
                  const mygame::person& rhs) const {
    return lhs.id == rhs.id() && lhs.name == rhs.name() &&
           lhs.age == rhs.age() &&
           std::abs(lhs.salary - rhs.salary()) < 0.0005f;
  }
};
template <>
struct PB_equal<struct_pb_sample::persons, mygame::persons> {
  bool operator()(const struct_pb_sample::persons& lhs,
                  const mygame::persons& rhs) const {
    return lhs.person_list == rhs.person_list();
  }
};
template <>
struct PB_equal<struct_pb_sample::Weapon, mygame::Weapon> {
  bool operator()(const struct_pb_sample::Weapon& lhs,
                  const mygame::Weapon& rhs) const {
    return lhs.name == rhs.name() && lhs.damage == rhs.damage();
  }
};
bool operator==(const struct_pb_sample::Monster::Color& a,
                const mygame::Monster::Color& b) {
  return int(a) == int(b);
}
template <>
struct PB_equal<struct_pb_sample::Monster, mygame::Monster> {
  bool operator()(const struct_pb_sample::Monster& lhs,
                  const mygame::Monster& rhs) const {
    auto ok = (lhs.name == rhs.name() && lhs.mana == rhs.mana() &&
               lhs.hp == rhs.hp() && lhs.inventory == rhs.inventory() &&
               lhs.color == rhs.color() && lhs.weapons == rhs.weapons() &&
               lhs.path == rhs.path());
    if (!ok) {
      return false;
    }
    if (lhs.pos && rhs.has_pos()) {
      ok =
          PB_equal<struct_pb_sample::Vec3, mygame::Vec3>()(*lhs.pos, rhs.pos());
      if (!ok) {
        return false;
      }
    }
    else if (lhs.pos || rhs.has_pos()) {
      return false;
    }
    if (lhs.equipped && rhs.has_equipped()) {
      ok = PB_equal<struct_pb_sample::Weapon, mygame::Weapon>()(*lhs.equipped,
                                                                rhs.equipped());
      if (!ok) {
        return false;
      }
    }
    else if (lhs.equipped || rhs.has_equipped()) {
      return false;
    }
    return true;
  }
};
template <>
struct PB_equal<struct_pb_sample::Monsters, mygame::Monsters> {
  bool operator()(const struct_pb_sample::Monsters& lhs,
                  const mygame::Monsters& rhs) const {
    return lhs.monsters == rhs.monsters();
  }
};
template <typename T, typename U>
void check_repeated(const std::vector<T>& a,
                    const google::protobuf::RepeatedPtrField<U>& b) {
  bool ret = (a == b);
  REQUIRE(ret);
}
#endif

TEST_SUITE_BEGIN("test pb benchmark struct");
TEST_CASE("testing rect") {
  auto my_rects = struct_pb_sample::create_rects(OBJECT_COUNT);
#ifdef HAVE_PROTOBUF
  auto pb_rects = protobuf_sample::create_rects(OBJECT_COUNT);
#endif
  for (int i = 0; i < OBJECT_COUNT; ++i) {
    auto my_rect = my_rects.rect32_list[i];
    check_self(my_rect);
#ifdef HAVE_PROTOBUF
    auto pb_rect = *(pb_rects.rect32_list().begin() + i);
    REQUIRE(my_rect.x == pb_rect.x());
    REQUIRE(my_rect.y == pb_rect.y());
    REQUIRE(my_rect.width == pb_rect.width());
    REQUIRE(my_rect.height == pb_rect.height());
    check_with_protobuf(my_rect, pb_rect);
#endif
  }
  check_self(my_rects);
#ifdef HAVE_PROTOBUF
  check_with_protobuf(my_rects, pb_rects);
#endif
}

TEST_CASE("testing person") {
  auto my_persons = struct_pb_sample::create_persons(OBJECT_COUNT);
#ifdef HAVE_PROTOBUF
  auto pb_persons = protobuf_sample::create_persons(OBJECT_COUNT);
#endif
  for (int i = 0; i < OBJECT_COUNT; ++i) {
    auto my_person = my_persons.person_list[i];
    check_self(my_person);
#ifdef HAVE_PROTOBUF
    auto pb_person = *(pb_persons.person_list().begin() + i);
    REQUIRE(my_person.id == pb_person.id());
    REQUIRE(my_person.name == pb_person.name());
    REQUIRE(my_person.age == pb_person.age());
    REQUIRE(my_person.salary == pb_person.salary());
    check_with_protobuf(my_person, pb_person);
#endif
  }
  check_self(my_persons);
#ifdef HAVE_PROTOBUF
  check_with_protobuf(my_persons, pb_persons);
#endif
}

TEST_CASE("testing monsters") {
  auto my_ms = struct_pb_sample::create_monsters(OBJECT_COUNT);
#ifdef HAVE_PROTOBUF
  auto pb_ms = protobuf_sample::create_monsters(OBJECT_COUNT);
#endif
  for (int i = 0; i < OBJECT_COUNT; ++i) {
    auto& my_m = my_ms.monsters[i];
    check_self(my_m);

#ifdef HAVE_PROTOBUF
    auto pb_m = *(pb_ms.monsters().begin() + i);
    REQUIRE(my_m.pos);
    REQUIRE(my_m.mana == pb_m.mana());
    REQUIRE(my_m.hp == pb_m.hp());
    REQUIRE(my_m.name == pb_m.name());
    REQUIRE(my_m.inventory == pb_m.inventory());
    REQUIRE(my_m.color == pb_m.color());
    check_repeated(my_m.weapons, pb_m.weapons());
    check_repeated(my_m.path, pb_m.path());

    check_with_protobuf(my_m, pb_m);
#endif
    auto sz = get_needed_size(my_m);
    std::string my_buf;
    my_buf.resize(sz);
    serialize_to(my_buf.data(), my_buf.size(), my_m);

    struct_pb_sample::Monster d_t{};
    auto ok = deserialize_to(d_t, my_buf.data(), my_buf.size());
    REQUIRE(ok);
    CHECK(d_t == my_m);
    REQUIRE(d_t.pos);
    REQUIRE(my_m.pos);
    CHECK(*d_t.pos == *my_m.pos);
    CHECK(d_t.mana == my_m.mana);
    CHECK(d_t.hp == my_m.hp);
    CHECK(d_t.name == my_m.name);
    CHECK(d_t.inventory == my_m.inventory);
    CHECK(d_t.color == my_m.color);
    REQUIRE(d_t.equipped);
    REQUIRE(my_m.equipped);
    CHECK(*d_t.equipped == *my_m.equipped);
    CHECK(d_t.path == my_m.path);
  }
  check_self(my_ms);
#ifdef HAVE_PROTOBUF
  check_with_protobuf(my_ms, pb_ms);
#endif
}
TEST_SUITE_END;
