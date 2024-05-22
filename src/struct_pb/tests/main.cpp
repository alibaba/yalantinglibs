#define DOCTEST_CONFIG_IMPLEMENT
#include <iostream>

#include "doctest.h"
#include "unittest_proto3.h"

#if defined(STRUCT_PB_WITH_PROTO)
TEST_CASE("test BaseTypeMsg") {
  {  // normal test
    stpb::BaseTypeMsg se_st{0,     100,  200,   300,     400,
                            31.4f, 62.8, false, "World", stpb::Enum::ZERO};
    std::string st_ss;
    struct_pb::to_pb(se_st, st_ss);

    pb::BaseTypeMsg se_msg;
    SetBaseTypeMsg(se_st, se_msg);
    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);
    CHECK(st_ss == pb_ss);

    stpb::BaseTypeMsg dese_st{};
    struct_pb::from_pb(dese_st, st_ss);
    pb::BaseTypeMsg dese_msg;
    dese_msg.ParseFromString(pb_ss);
    CheckBaseTypeMsg(dese_st, dese_msg);
  }

  {  // test min and empty str
    stpb::BaseTypeMsg se_st{0,
                            std::numeric_limits<int32_t>::min(),
                            std::numeric_limits<int64_t>::min(),
                            std::numeric_limits<uint32_t>::min(),
                            std::numeric_limits<uint64_t>::min(),
                            std::numeric_limits<float>::lowest(),
                            std::numeric_limits<double>::lowest(),
                            false,
                            "",
                            stpb::Enum::NEG};
    std::string st_ss;
    struct_pb::to_pb(se_st, st_ss);

    pb::BaseTypeMsg se_msg;
    SetBaseTypeMsg(se_st, se_msg);
    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);
    CHECK(st_ss == pb_ss);

    stpb::BaseTypeMsg dese_st{};
    struct_pb::from_pb(dese_st, st_ss);
    pb::BaseTypeMsg dese_msg;
    dese_msg.ParseFromString(pb_ss);
    CheckBaseTypeMsg(dese_st, dese_msg);
  }
  {  // test max and long str
    stpb::BaseTypeMsg se_st{0,
                            std::numeric_limits<int32_t>::max(),
                            std::numeric_limits<int64_t>::max(),
                            std::numeric_limits<uint32_t>::max(),
                            std::numeric_limits<uint64_t>::max(),
                            std::numeric_limits<float>::max(),
                            std::numeric_limits<double>::max(),
                            true,
                            std::string(1000, 'x'),
                            stpb::Enum::BAZ};
    std::string st_ss;
    struct_pb::to_pb(se_st, st_ss);

    pb::BaseTypeMsg se_msg;
    SetBaseTypeMsg(se_st, se_msg);
    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);
    CHECK(st_ss == pb_ss);

    stpb::BaseTypeMsg dese_st{};
    struct_pb::from_pb(dese_st, st_ss);
    pb::BaseTypeMsg dese_msg;
    dese_msg.ParseFromString(pb_ss);
    CheckBaseTypeMsg(dese_st, dese_msg);
  }
}

TEST_CASE("test person and monster") {
  stpb::simple_t2 t{0, -100, 2, stpb::Color::Blue, 4};
  std::string str;
  struct_pb::to_pb(t, str);

  stpb::simple_t2 t2;
  struct_pb::from_pb(t2, str);
  CHECK(t.c == t2.c);

  pb::Simple2 s;
  s.set_a(-100);
  s.set_b(2);
  s.set_c(pb::Color::Blue);
  s.set_d(4);

  std::string pb_str;
  s.SerializeToString(&pb_str);

  CHECK(str == pb_str);
}

TEST_CASE("test person and monster") {
  auto pb_monster = protobuf_sample::create_monster();
  auto sp_monster = create_sp_monster();

  std::string pb_str;
  std::string sp_str;

  pb_monster.SerializeToString(&pb_str);
  struct_pb::to_pb(sp_monster, sp_str);

  CHECK(pb_str == sp_str);

  mygame::Monster m;
  m.ParseFromString(pb_str);
  CHECK(m.name() == pb_monster.name());

  stpb::Monster spm;
  struct_pb::from_pb(spm, sp_str);
  CHECK(spm.name == sp_monster.name);

  auto pb_person = protobuf_sample::create_person();
  auto sp_person = create_person();
  pb_person.SerializePartialToString(&pb_str);
  struct_pb::to_pb(sp_person, sp_str);

  CHECK(pb_str == sp_str);

  mygame::person pp;
  pp.ParseFromString(pb_str);
  CHECK(pp.name() == pb_person.name());

  stpb::person p;
  struct_pb::from_pb(p, sp_str);
  CHECK(p.name == sp_person.name);
}

TEST_CASE("test IguanaTypeMsg") {
  {  // test normal value
    stpb::IguanaTypeMsg se_st{0, {100}, {200}, {300}, {400}, {31}, {32}};
    std::string st_ss;
    struct_pb::to_pb(se_st, st_ss);

    pb::IguanaTypeMsg se_msg{};
    SetIguanaTypeMsg(se_st, se_msg);
    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);
    CHECK(st_ss == pb_ss);

    stpb::IguanaTypeMsg dese_st{};
    struct_pb::from_pb(dese_st, st_ss);
    pb::IguanaTypeMsg dese_msg;
    dese_msg.ParseFromString(pb_ss);
    CheckIguanaTypeMsg(dese_st, dese_msg);
  }

  {  // test min value
    stpb::IguanaTypeMsg se_st{0,
                              {std::numeric_limits<int32_t>::min()},
                              {std::numeric_limits<int64_t>::min()},
                              {std::numeric_limits<uint32_t>::min()},
                              {std::numeric_limits<uint64_t>::min()},
                              {std::numeric_limits<int32_t>::lowest()},
                              {std::numeric_limits<int64_t>::lowest()}};
    std::string st_ss;
    struct_pb::to_pb(se_st, st_ss);

    pb::IguanaTypeMsg se_msg{};
    SetIguanaTypeMsg(se_st, se_msg);
    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);
    CHECK(st_ss == pb_ss);
    stpb::IguanaTypeMsg dese_st{};
    struct_pb::from_pb(dese_st, st_ss);
    pb::IguanaTypeMsg dese_msg;
    dese_msg.ParseFromString(pb_ss);
    CheckIguanaTypeMsg(dese_st, dese_msg);
  }
  {  // test max value
    stpb::IguanaTypeMsg se_st{0,
                              {std::numeric_limits<int32_t>::max()},
                              {std::numeric_limits<int64_t>::max()},
                              {std::numeric_limits<uint32_t>::max()},
                              {std::numeric_limits<uint64_t>::max()},
                              {std::numeric_limits<int32_t>::max()},
                              {std::numeric_limits<int64_t>::max()}};
    std::string st_ss;
    struct_pb::to_pb(se_st, st_ss);

    pb::IguanaTypeMsg se_msg;
    SetIguanaTypeMsg(se_st, se_msg);
    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);
    CHECK(st_ss == pb_ss);

    stpb::IguanaTypeMsg dese_st{};
    struct_pb::from_pb(dese_st, st_ss);
    pb::IguanaTypeMsg dese_msg;
    dese_msg.ParseFromString(pb_ss);
    CheckIguanaTypeMsg(dese_st, dese_msg);
  }
  {  // test empty
    stpb::IguanaTypeMsg se_st{};
    std::string st_ss;
    struct_pb::to_pb(se_st, st_ss);

    pb::IguanaTypeMsg se_msg;
    SetIguanaTypeMsg(se_st, se_msg);
    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);
    CHECK(st_ss == pb_ss);

    stpb::IguanaTypeMsg dese_st{};
    struct_pb::from_pb(dese_st, st_ss);
    pb::IguanaTypeMsg dese_msg;
    dese_msg.ParseFromString(pb_ss);
    CheckIguanaTypeMsg(dese_st, dese_msg);
  }
}

TEST_CASE("test RepeatBaseTypeMsg") {
  {
    stpb::RepeatBaseTypeMsg se_st{
        0,
        {1, 2, 3},
        {4, 5, 6},
        {7, 8, 9},
        {10, 11, 12},
        {13.1, 14.2, 15.3},
        {16.4, 17.5, 18.6},
        {"a", "b", "c"},
        {stpb::Enum::BAZ, stpb::Enum::ZERO, stpb::Enum::NEG}};
    std::string st_ss;
    struct_pb::to_pb(se_st, st_ss);

    pb::RepeatBaseTypeMsg se_msg;
    SetRepeatBaseTypeMsg(se_st, se_msg);
    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);
    CHECK(st_ss == pb_ss);

    stpb::RepeatBaseTypeMsg dese_st{};
    struct_pb::from_pb(dese_st, st_ss);
    pb::RepeatBaseTypeMsg dese_msg;
    dese_msg.ParseFromString(pb_ss);
    CheckRepeatBaseTypeMsg(dese_st, dese_msg);
  }
  {  // max and min vlaue
    stpb::RepeatBaseTypeMsg se_st{0,
                                  {std::numeric_limits<uint32_t>::max(),
                                   std::numeric_limits<uint32_t>::min()},
                                  {std::numeric_limits<uint64_t>::max(),
                                   std::numeric_limits<uint64_t>::min()},
                                  {std::numeric_limits<int32_t>::max(),
                                   std::numeric_limits<int32_t>::min()},
                                  {std::numeric_limits<int64_t>::max(),
                                   std::numeric_limits<int64_t>::min()},
                                  {},
                                  {std::numeric_limits<double>::max(),
                                   std::numeric_limits<double>::min()},
                                  {"", "", ""},
                                  {stpb::Enum::NEG, stpb::Enum::FOO}};
    std::string st_ss;
    struct_pb::to_pb(se_st, st_ss);

    pb::RepeatBaseTypeMsg se_msg;
    SetRepeatBaseTypeMsg(se_st, se_msg);
    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);
    CHECK(st_ss == pb_ss);
    stpb::RepeatBaseTypeMsg dese_st{};
    struct_pb::from_pb(dese_st, st_ss);
    pb::RepeatBaseTypeMsg dese_msg;
    dese_msg.ParseFromString(pb_ss);
    CheckRepeatBaseTypeMsg(dese_st, dese_msg);
  }
}

TEST_CASE("test RepeatIguanaTypeMsg") {
  {
    stpb::RepeatIguanaTypeMsg se_st{
        0,
        {{0}, {1}, {3}},
        {{4}, {5}, {6}},
        {{7}, {8}, {9}},
        {{10}, {11}, {12}},
        {{13}, {14}, {15}},
        {},
    };
    std::string st_ss;
    struct_pb::to_pb(se_st, st_ss);

    pb::RepeatIguanaTypeMsg se_msg;
    SetRepeatIguanaTypeMsg(se_st, se_msg);
    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);
    CHECK(st_ss == pb_ss);

    stpb::RepeatIguanaTypeMsg dese_st{};
    struct_pb::from_pb(dese_st, st_ss);
    pb::RepeatIguanaTypeMsg dese_msg;
    dese_msg.ParseFromString(pb_ss);
    CheckRepeatIguanaTypeMsg(dese_st, dese_msg);
  }
}

TEST_CASE("test NestedMsg") {
  {
    stpb::NestedMsg se_st{
        0,
        /* base_msg */
        {0, 100, 200, 300, 400, 31.4f, 62.8, false, "World", stpb::Enum::BAZ},
        /* repeat_base_msg */
        {{0, 1, 2, 3, 4, 5.5f, 6.6, true, "Hello", stpb::Enum::FOO},
         {0, 7, 8, 9, 10, 11.11f, 12.12, false, "Hi", stpb::Enum::BAR}},
        /* iguana_type_msg */ {0, {100}, {200}, {300}, {400}, {31}, {32}},
        /* repeat_iguna_msg */
        {{0, {1}, {2}, {3}}, {0, {4}, {5}, {6}}, {0, {7}, {8}, {9}}},
        /* repeat_repeat_base_msg */
        {{0,
          {1, 2, 3},
          {4, 5, 6},
          {7, 8, 9},
          {10, 11, 12},
          {13.1, 14.2, 15.3},
          {16.4, 17.5, 18.6},
          {"a", "b", "c"},
          {stpb::Enum::FOO, stpb::Enum::BAR, stpb::Enum::BAZ}},
         {0,
          {19, 20, 21},
          {22, 23, 24},
          {25, 26, 27},
          {28, 29, 30},
          {31.1, 32.2, 33.3},
          {34.4, 35.5, 36.6},
          {"x", "y", "z"},
          {stpb::Enum::ZERO, stpb::Enum::NEG, stpb::Enum::FOO}}}};

    std::string st_ss;
    struct_pb::to_pb(se_st, st_ss);

    pb::NestedMsg se_msg;
    SetNestedMsg(se_st, se_msg);

    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);

    CHECK(st_ss == pb_ss);

    stpb::NestedMsg dese_st{};
    struct_pb::from_pb(dese_st, st_ss);

    pb::NestedMsg dese_msg;
    dese_msg.ParseFromString(pb_ss);

    CheckNestedMsg(dese_st, dese_msg);
  }
  {  // test empty values
    stpb::NestedMsg se_st{
        0,
        /* base_msg */ {0, 0, 0, 0, 0, 0.0f, 0.0, true, "", stpb::Enum::ZERO},
        /* repeat_base_msg */ {},
        /* iguana_type_msg */ {},
        /* repeat_iguna_msg */ {},
        /* repeat_repeat_base_msg */ {}};
    std::string st_ss;
    struct_pb::to_pb(se_st, st_ss);

    pb::NestedMsg se_msg;
    SetNestedMsg(se_st, se_msg);
    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);

    CHECK(st_ss == pb_ss);
    print_hex_str(st_ss);
    print_hex_str(pb_ss);
    stpb::NestedMsg dese_st{};
    struct_pb::from_pb(dese_st, st_ss);

    pb::NestedMsg dese_msg;
    dese_msg.ParseFromString(pb_ss);

    CheckNestedMsg(dese_st, dese_msg);
  }
}

TEST_CASE("test MapMsg") {
  {
    stpb::MapMsg se_st{};

    se_st.sfix64_str_map.emplace(struct_pb::sfixed64_t{10}, "ten");
    se_st.sfix64_str_map.emplace(struct_pb::sfixed64_t{20}, "twenty");

    se_st.str_iguana_type_msg_map.emplace(
        "first", stpb::IguanaTypeMsg{{10}, {20}, {30}, {40}, {50}, {60}});
    se_st.str_iguana_type_msg_map.emplace(
        "second", stpb::IguanaTypeMsg{{11}, {21}, {31}, {41}, {51}, {61}});

    se_st.int_repeat_base_msg_map.emplace(
        1, stpb::RepeatBaseTypeMsg{0,
                                   {1, 2},
                                   {3, 4},
                                   {5, 6},
                                   {7, 8},
                                   {9.0f, 10.0f},
                                   {11.0, 12.0},
                                   {"one", "two"},
                                   {stpb::Enum::FOO, stpb::Enum::BAR}});
    se_st.int_repeat_base_msg_map.emplace(
        2, stpb::RepeatBaseTypeMsg{0,
                                   {2, 3},
                                   {4, 5},
                                   {6, 7},
                                   {8, 9},
                                   {10.0f, 11.0f},
                                   {12.0, 13.0},
                                   {"three", "four"},
                                   {stpb::Enum::BAZ, stpb::Enum::NEG}});

    std::string st_ss;
    struct_pb::to_pb(se_st, st_ss);

    pb::MapMsg se_msg{};
    SetMapMsg(se_st, se_msg);
    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);
    // It's okay not to satisfy this.
    // CHECK(st_ss == pb_ss);
    CHECK(st_ss.size() == pb_ss.size());
    stpb::MapMsg dese_st{};
    struct_pb::from_pb(dese_st, pb_ss);
    pb::MapMsg dese_msg;
    dese_msg.ParseFromString(st_ss);
    CheckMapMsg(dese_st, dese_msg);
  }
  {
    // key empty
    stpb::MapMsg se_st{};
    se_st.sfix64_str_map.emplace(struct_pb::sfixed64_t{30}, "");
    se_st.str_iguana_type_msg_map.emplace(
        "", stpb::IguanaTypeMsg{0, {0}, {0}, {0}, {0}, {0}, {0}});
    se_st.int_repeat_base_msg_map.emplace(
        3, stpb::RepeatBaseTypeMsg{0, {}, {}, {}, {}, {}, {}, {}, {}});
    std::string st_ss;
    struct_pb::to_pb(se_st, st_ss);

    pb::MapMsg se_msg{};
    SetMapMsg(se_st, se_msg);
    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);
    CHECK(st_ss == pb_ss);

    stpb::MapMsg dese_st{};
    struct_pb::from_pb(dese_st, pb_ss);
    pb::MapMsg dese_msg;
    dese_msg.ParseFromString(st_ss);
    CheckMapMsg(dese_st, dese_msg);
  }
}

TEST_CASE("test BaseOneofMsg") {
  {  // test double
    stpb::BaseOneofMsg se_st{0, 123, 3.14159, 456.78};
    std::string st_ss;
    struct_pb::to_pb(se_st, st_ss);

    pb::BaseOneofMsg se_msg;
    SetBaseOneofMsg(se_st, se_msg);
    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);
    CHECK(st_ss == pb_ss);
    // print_hex_str(st_ss);
    // print_hex_str(pb_ss);
    stpb::BaseOneofMsg dese_st{};
    struct_pb::from_pb(dese_st, st_ss);

    pb::BaseOneofMsg dese_msg;
    dese_msg.ParseFromString(pb_ss);
    CheckBaseOneofMsg(dese_st, dese_msg);
  }
  {  // test string
    stpb::BaseOneofMsg se_st{0, 123, std::string("Hello"), 456.78};
    std::string st_ss;
    struct_pb::to_pb(se_st, st_ss);

    pb::BaseOneofMsg se_msg;
    SetBaseOneofMsg(se_st, se_msg);
    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);
    CHECK(st_ss == pb_ss);

    stpb::BaseOneofMsg dese_st{};
    struct_pb::from_pb(dese_st, st_ss);

    pb::BaseOneofMsg dese_msg;
    dese_msg.ParseFromString(pb_ss);
    CheckBaseOneofMsg(dese_st, dese_msg);
  }
  {  // test BaseTypeMsg
    stpb::BaseTypeMsg baseTypeMsg{0,     100,  200,   300,     400,
                                  31.4f, 62.8, false, "World", stpb::Enum::BAZ};
    stpb::BaseOneofMsg se_st{0, 123, baseTypeMsg, 456.78};

    std::string st_ss;
    struct_pb::to_pb(se_st, st_ss);

    pb::BaseOneofMsg se_msg;
    SetBaseOneofMsg(se_st, se_msg);
    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);
    CHECK(st_ss == pb_ss);

    stpb::BaseOneofMsg dese_st{};
    struct_pb::from_pb(dese_st, st_ss);

    pb::BaseOneofMsg dese_msg;
    dese_msg.ParseFromString(pb_ss);
    CheckBaseOneofMsg(dese_st, dese_msg);
  }
  {  // test empty variant
    stpb::BaseOneofMsg se_st{0, 123, {}, 456.78};

    std::string st_ss;
    struct_pb::to_pb(se_st, st_ss);

    pb::BaseOneofMsg se_msg;
    SetBaseOneofMsg(se_st, se_msg);
    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);
    CHECK(st_ss == pb_ss);
    print_hex_str(st_ss);
    print_hex_str(pb_ss);
    stpb::BaseOneofMsg dese_st{};
    struct_pb::from_pb(dese_st, st_ss);

    pb::BaseOneofMsg dese_msg;
    dese_msg.ParseFromString(pb_ss);
    CheckBaseOneofMsg(dese_st, dese_msg);
  }
}

TEST_CASE("test NestOneofMsg ") {
  {  // Test BaseOneofMsg
    stpb::BaseOneofMsg baseOneof{0, 123, std::string("Hello"), 456.78};
    stpb::NestOneofMsg se_st{{baseOneof}};

    std::string st_ss;
    struct_pb::to_pb(se_st, st_ss);

    pb::NestOneofMsg se_msg;
    SetNestOneofMsg(se_st, se_msg);
    std::string pb_ss;
    se_msg.SerializeToString(&pb_ss);
    CHECK(st_ss == pb_ss);
    stpb::NestOneofMsg dese_st{};
    struct_pb::from_pb(dese_st, st_ss);

    pb::NestOneofMsg dese_msg;
    dese_msg.ParseFromString(pb_ss);
    CheckNestOneofMsg(dese_st, dese_msg);
  }
}
#endif

struct point_t PUBLIC {
  int x;
  double y;
};
REFLECTION(point_t, x, y);

namespace my_space {
struct inner_struct PUBLIC {
  int x;
  int y;
  int z;
};

constexpr inline auto get_members_impl(inner_struct *) {
  return std::make_tuple(struct_pb::field_t{&inner_struct::x, 7, "a"},
                         struct_pb::field_t{&inner_struct::y, 9, "b"},
                         struct_pb::field_t{&inner_struct::z, 12, "c"});
}
}  // namespace my_space

struct test_pb_st1 PUBLIC {
  int x;
  struct_pb::sint32_t y;
  struct_pb::sint64_t z;
};
REFLECTION(test_pb_st1, x, y, z);

struct test_pb_st2 PUBLIC {
  int x;
  struct_pb::fixed32_t y;
  struct_pb::fixed64_t z;
};
REFLECTION(test_pb_st2, x, y, z);

struct test_pb_st3 PUBLIC {
  int x;
  struct_pb::sfixed32_t y;
  struct_pb::sfixed64_t z;
};
REFLECTION(test_pb_st3, x, y, z);

struct test_pb_st4 PUBLIC {
  int x;
  std::string y;
};
REFLECTION(test_pb_st4, x, y);

struct test_pb_st5 PUBLIC {
  int x;
  std::string_view y;
};
REFLECTION(test_pb_st5, x, y);

struct test_pb_st6 PUBLIC {
  std::optional<int> x;
  std::optional<std::string> y;
};
REFLECTION(test_pb_st6, x, y);

struct pair_t PUBLIC {
  int x;
  int y;
};
REFLECTION(pair_t, x, y);

struct message_t PUBLIC {
  int id;
  pair_t t;
};
REFLECTION(message_t, id, t);

struct test_pb_st8 PUBLIC {
  int x;
  pair_t y;
  message_t z;
};
REFLECTION(test_pb_st8, x, y, z);

struct test_pb_st9 PUBLIC {
  int x;
  std::vector<int> y;
  std::string z;
};
REFLECTION(test_pb_st9, x, y, z);

struct test_pb_st10 PUBLIC {
  int x;
  std::vector<message_t> y;
  std::string z;
};
REFLECTION(test_pb_st10, x, y, z);

struct test_pb_st11 PUBLIC {
  int x;
  std::vector<std::optional<message_t>> y;
  std::vector<std::string> z;
};
REFLECTION(test_pb_st11, x, y, z);

struct test_pb_st12 PUBLIC {
  int x;
  std::map<int, std::string> y;
  std::map<std::string, int> z;
};
REFLECTION(test_pb_st12, x, y, z);

struct test_pb_st13 PUBLIC {
  int x;
  std::map<int, message_t> y;
  std::string z;
};
REFLECTION(test_pb_st13, x, y, z);

enum class colors_t { red, black };

enum level_t { debug, info };

struct test_pb_st14 PUBLIC {
  int x;
  colors_t y;
  level_t z;
};
REFLECTION(test_pb_st14, x, y, z);

namespace client {
struct person PUBLIC {
  std::string name;
  int64_t age;
};

REFLECTION(person, name, age);
}  // namespace client

struct my_struct PUBLIC {
  int x;
  bool y;
  struct_pb::fixed64_t z;
};
REFLECTION(my_struct, x, y, z);

struct nest1 PUBLIC {
  std::string name;
  my_struct value;
  int var;
};

REFLECTION(nest1, name, value, var);

struct numer_st PUBLIC {
  bool a;
  double b;
  float c;
};
REFLECTION(numer_st, a, b, c);

TEST_CASE("test struct_pb") {
  {
    my_space::inner_struct inner{0, 41, 42, 43};

    std::string str;
    struct_pb::to_pb(inner, str);
    CHECK(str.size() == struct_pb::detail::pb_key_value_size<0>(inner));

    my_space::inner_struct inner1;
    struct_pb::from_pb(inner1, str);
    CHECK(inner.x == inner1.x);
    CHECK(inner.y == inner1.y);
    CHECK(inner.z == inner1.z);
  }

  {
    test_pb_st1 st1{0, 41, {42}, {43}};
    std::string str;
    struct_pb::to_pb(st1, str);
    CHECK(str.size() == struct_pb::detail::pb_key_value_size<0>(st1));

    test_pb_st1 st2;
    struct_pb::from_pb(st2, str);
    CHECK(st1.x == st2.x);
    CHECK(st1.y.val == st2.y.val);
    CHECK(st1.z.val == st2.z.val);
  }

  {
    test_pb_st2 st1{0, 41, {42}, {43}};
    std::string str;
    struct_pb::to_pb(st1, str);
    CHECK(str.size() == struct_pb::detail::pb_key_value_size<0>(st1));

    test_pb_st2 st2;
    struct_pb::from_pb(st2, str);
    CHECK(st1.y.val == st2.y.val);
  }
  {
    test_pb_st3 st1{0, 41, {42}, {43}};
    std::string str;
    struct_pb::to_pb(st1, str);
    CHECK(str.size() == struct_pb::detail::pb_key_value_size<0>(st1));

    test_pb_st3 st2;
    struct_pb::from_pb(st2, str);
    CHECK(st1.y.val == st2.y.val);
  }
  {
    test_pb_st4 st1{0, 41, "it is a test"};
    std::string str;
    struct_pb::to_pb(st1, str);
    CHECK(str.size() == struct_pb::detail::pb_key_value_size<0>(st1));

    test_pb_st4 st2;
    struct_pb::from_pb(st2, str);
    CHECK(st1.y == st2.y);
  }

  {
    test_pb_st5 st1{0, 41, "it is a test"};
    std::string str;
    struct_pb::to_pb(st1, str);
    CHECK(str.size() == struct_pb::detail::pb_key_value_size<0>(st1));

    test_pb_st5 st2;
    struct_pb::from_pb(st2, str);
    CHECK(st1.y == st2.y);
  }
  {
    // optional
    test_pb_st6 st1{0, 41, "it is a test"};
    std::string str;
    struct_pb::to_pb(st1, str);
    CHECK(str.size() == struct_pb::detail::pb_key_value_size<0>(st1));

    test_pb_st6 st2;
    struct_pb::from_pb(st2, str);
    CHECK(st1.y == st2.y);
  }
  {
    // sub nested objects
    nest1 v{0, "Hi", {0, 1, false, {3}}, 5}, v2{};
    std::string s;
    struct_pb::to_pb(v, s);
    struct_pb::from_pb(v2, s);
    CHECK(v.var == v2.var);
    CHECK(v.value.y == v2.value.y);
    CHECK(v.value.z == v2.value.z);

    test_pb_st8 st1{0, 1, {0, 3, 4}, {0, 5, {0, 7, 8}}};
    std::string str;
    struct_pb::to_pb(st1, str);

    test_pb_st8 st2;
    struct_pb::from_pb(st2, str);
    CHECK(st1.z.t.y == st2.z.t.y);
  }

  {
    // repeated messages
    test_pb_st9 st1{0, 1, {2, 4, 6}, "test"};
    std::string str;
    struct_pb::to_pb(st1, str);
    CHECK(str.size() == struct_pb::detail::pb_key_value_size<0>(st1));

    test_pb_st9 st2;
    struct_pb::from_pb(st2, str);
    CHECK(st1.z == st2.z);
  }
  {
    test_pb_st10 st1{0, 1, {{0, 5, {7, 8}}, {0, 9, {11, 12}}}, "test"};
    std::string str;
    struct_pb::to_pb(st1, str);

    test_pb_st10 st2;
    struct_pb::from_pb(st2, str);
    CHECK(st1.z == st2.z);
  }
  {
    message_t m{0, 1, {0, 3, 4}};
    test_pb_st11 st1{0, 1, {m}, {}};
    std::string str;
    struct_pb::to_pb(st1, str);

    test_pb_st11 st2;
    struct_pb::from_pb(st2, str);
    CHECK(st1.z == st2.z);
  }
  {
    message_t st1{};
    std::string str;
    struct_pb::to_pb(st1, str);

    message_t st2{};
    struct_pb::from_pb(st2, str);
    CHECK(st1.id == st2.id);
  }
  {
    test_pb_st11 st1{0, 1, {{{0, 5, {7, 8}}}, {{0, 9, {11, 12}}}}, {"test"}};
    std::string str;
    struct_pb::to_pb(st1, str);

    test_pb_st11 st2;
    struct_pb::from_pb(st2, str);
    CHECK(st1.z == st2.z);
  }
  {
    // map messages
    test_pb_st12 st1{0, 1, {{1, "test"}, {2, "ok"}}, {{"test", 4}, {"ok", 6}}};
    std::string str;
    struct_pb::to_pb(st1, str);

    test_pb_st12 st2;
    struct_pb::from_pb(st2, str);
    CHECK(st1.z == st2.z);
  }
  {
    // map messages
    test_pb_st12 st1{0, 1, {{1, ""}, {0, "ok"}}, {{"", 4}, {"ok", 0}}};
    std::string str;
    struct_pb::to_pb(st1, str);  // error
    print_hex_str(str);
    test_pb_st12 st2;
    struct_pb::from_pb(st2, str);
    CHECK(st1.z == st2.z);
  }
  {
    // map messages
    test_pb_st13 st1;
    st1.x = 1;
    st1.y.emplace(1, message_t{0, 2, {0, 3, 4}});
    st1.y.emplace(2, message_t{0, 4, {0, 6, 8}});
    st1.z = "test";
    std::string str;
    struct_pb::to_pb(st1, str);

    test_pb_st13 st2;
    struct_pb::from_pb(st2, str);
    CHECK(st1.z == st2.z);
  }
  {
    // map messages
    test_pb_st13 st1;
    st1.x = 1;
    st1.y.emplace(2, message_t{});
    st1.y.emplace(3, message_t{});
    st1.z = "test";
    std::string str;
    struct_pb::to_pb(st1, str);

    test_pb_st13 st2;
    struct_pb::from_pb(st2, str);
    CHECK(st1.z == st2.z);
  }
  {
    // enum
    test_pb_st14 st1{0, 1, colors_t::black, level_t::info};
    std::string str;
    struct_pb::to_pb(st1, str);

    test_pb_st14 st2;
    struct_pb::from_pb(st2, str);
    CHECK(st1.z == st2.z);
  }
  {
    // bool float double
    numer_st n{0, true, 10.25, 4.578}, n1;
    std::string str;
    struct_pb::to_pb(n, str);

    struct_pb::from_pb(n1, str);
    CHECK(n1.a == n.a);
    CHECK(n1.b == n.b);
    CHECK(n1.c == n.c);
  }
}

TEST_CASE("test members") {
  using namespace struct_pb;
  using namespace struct_pb::detail;

  my_space::inner_struct inner{0, 41, 42, 43};
  const auto &map = struct_pb::get_members<my_space::inner_struct>();
  std::visit(
      [&inner](auto &member) mutable {
        CHECK(member.field_no == 9);
        CHECK(member.field_name == "b");
        CHECK(member.value(inner) == 42);
      },
      map.at(9));

  point_t pt{0, 2, 3};
  const auto &arr1 = struct_pb::get_members<point_t>();
  auto &val = arr1.at(1);
  std::visit(
      [&pt](auto &member) mutable {
        CHECK(member.field_no == 1);
        CHECK(member.field_name == "x");
        CHECK(member.value(pt) == 2);
      },
      val);
}

struct test_variant PUBLIC {
  int x;
  std::variant<double, std::string, int> y;
  double z;
};
REFLECTION(test_variant, x, y, z);

TEST_CASE("test variant") {
  {
    constexpr auto tp = struct_pb::get_members_tuple<test_variant>();
    static_assert(std::get<0>(tp).field_no == 1);
    static_assert(std::get<1>(tp).field_no == 2);
    static_assert(std::get<2>(tp).field_no == 3);
    static_assert(std::get<3>(tp).field_no == 4);
    static_assert(std::get<4>(tp).field_no == 5);
  }
  {
    constexpr static auto map = struct_pb::get_members<test_variant>();
    static_assert(map.find(1) != map.end());
    static_assert(map.find(2) != map.end());
    static_assert(map.find(3) != map.end());
    static_assert(map.find(4) != map.end());
    auto val1 = map.find(2);
    auto val2 = map.find(3);
    std::visit(
        [](auto &member) mutable {
          CHECK(member.field_no == 2);
          CHECK(member.field_name == "y");
        },
        val1->second);
    std::visit(
        [](auto &member) mutable {
          CHECK(member.field_no == 3);
          CHECK(member.field_name == "y");
        },
        val2->second);
  }
  {
    test_variant st1 = {0, 5, "Hello, variant!", 3.14};
    std::string str;
    struct_pb::to_pb(st1, str);
    test_variant st2;
    struct_pb::from_pb(st2, str);
    CHECK(st1.z == st2.z);
    CHECK(std::get<std::string>(st2.y) == "Hello, variant!");
  }
  {
    test_variant st1 = {0, 5, 3.88, 3.14};
    std::string str;
    struct_pb::to_pb(st1, str);
    test_variant st2;
    struct_pb::from_pb(st2, str);
    CHECK(st1.z == st2.z);
    CHECK(std::get<double>(st2.y) == 3.88);
  }
}

// doctest comments
// 'function' : must be 'attribute' - see issue #182
DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007)
int main(int argc, char **argv) { return doctest::Context(argc, argv).run(); }
DOCTEST_MSVC_SUPPRESS_WARNING_POP