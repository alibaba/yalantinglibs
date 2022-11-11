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

#include "benchmark.pb.h"

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
