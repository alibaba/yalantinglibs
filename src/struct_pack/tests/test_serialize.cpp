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

#include <algorithm>
#include <cstddef>
#include <ratio>
#include <string_view>
#include <system_error>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

#include "struct_pack/struct_pack/reflection.h"
#define private public
#include "struct_pack/struct_pack.hpp"
#undef private
#include <array>
#include <deque>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <span>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <util/expected.hpp>
#include <vector>

#include "doctest.h"
using namespace struct_pack;

struct person {
  int age;
  std::string name;
  auto operator==(const person &rhs) const {
    return age == rhs.age && name == rhs.name;
  }
  bool operator<(person const &p) const {
    return age < p.age || (age == p.age && name < p.name);
  }
};

struct person1 {
  int age;
  std::string name;
  struct_pack::compatible<int32_t> id;
  struct_pack::compatible<bool> maybe;
};

struct empty {};

enum class Color { red, black, white };

struct complicated_object {
  Color color;
  int a;
  std::string b;
  std::vector<person> c;
  std::list<std::string> d;
  std::deque<int> e;
  std::map<int, person> f;
  std::multimap<int, person> g;
  std::set<std::string> h;
  std::multiset<int> i;
  std::unordered_map<int, person> j;
  std::unordered_multimap<int, int> k;
  std::array<person, 2> m;
  person n[2];
  std::pair<std::string, person> o;
};

struct nested_object {
  int id;
  std::string name;
  person p;
  complicated_object o;
};

TEST_CASE("testing deserialize_view") {
  person p{.age = 32, .name = "tom"};
  auto ret = serialize(p);
  {
    auto res = deserialize<person>(ret);
    CHECK(res);
    CHECK(res.value() == p);
  }
  {
    size_t len = 114514;
    auto res = deserialize<person>(ret, len);
    CHECK(res);
    CHECK(res.value() == p);
    CHECK(len == ret.size());
  }
  {
    size_t offset = 0;
    auto res = deserialize_with_offset<person>(ret, offset);
    CHECK(res);
    CHECK(res.value() == p);
    CHECK(offset == ret.size());
  }
  {
    person p2;
    auto res = deserialize_to(p2, ret);
    CHECK(res == std::errc{});
    CHECK(p2 == p);
  }
  {
    size_t len = 114514;
    person p2;
    auto res = deserialize_to(p2, ret, len);
    CHECK(res == std::errc{});
    CHECK(p2 == p);
    CHECK(len == ret.size());
  }
  {
    size_t offset = 0;
    person p2;
    auto res = deserialize_to(p2, ret, offset);
    CHECK(res == std::errc{});
    CHECK(p2 == p);
    CHECK(offset == ret.size());
  }
  {
    person p2;
    auto res = get_field<person, 1>(ret);
    CHECK(res);
    CHECK(res.value() == p.name);
  }
}

TEST_CASE("testing api") {
  person p{16, "jack"};
  int offset = 20;
  SUBCASE("return buffer") {
    auto ret1 = serialize(p);
    auto ret2 = serialize_with_offset(offset, p);
    // TODO: CHECK offset
    CHECK(ret1.size() == ret2.size() - offset);
    auto p1 = deserialize<person>(ret1.data(), ret1.size());
    CHECK(p1);
    auto p2 = deserialize<person>(ret2.data() + offset, ret1.size() - offset);
    CHECK(p2);
    CHECK(p == p1.value());
    CHECK(p == p2.value());
  }
  SUBCASE("serialize to") {
    auto need_size = get_needed_size(p);
    std::vector<std::byte> my_buffer, my_buffer2;
    my_buffer.resize(need_size);
    my_buffer2.resize(need_size + offset);
    auto ret1 = serialize_to(my_buffer.data(), need_size, p);
    CHECK(ret1 == need_size);
    auto ret2 = serialize_to(my_buffer2.data() + offset, need_size, p);
    CHECK(ret2 == need_size);
    person p1, p2;
    auto ec1 = deserialize_to(p1, my_buffer.data(), my_buffer.size());
    CHECK(ec1 == std::errc{});
    auto ec2 = deserialize_to(p2, my_buffer2.data() + offset,
                              my_buffer2.size() - offset);
    CHECK(ec2 == std::errc{});
    CHECK(p == p1);
    CHECK(p == p2);
  }
  SUBCASE("append buffer") {
    std::vector<unsigned char> my_buffer, my_buffer2;
    serialize_to(my_buffer, p);
    serialize_to_with_offset(my_buffer2, offset, p);
    CHECK(my_buffer.size() == my_buffer2.size() - offset);
    auto ret1 = get_field<person, 0>(my_buffer.data(), my_buffer.size());
    CHECK(ret1);
    auto ret2 = get_field<person, 0>(my_buffer2.data() + offset,
                                     my_buffer2.size() - offset);
    CHECK(ret2);
    CHECK(ret1.value() == p.age);
    CHECK(ret2.value() == p.age);
    auto ret3 = get_field<person, 1>(my_buffer.data(), my_buffer.size());
    CHECK(ret3);
    auto ret4 = get_field<person, 1>(my_buffer2.data() + offset,
                                     my_buffer2.size() - offset);
    CHECK(ret4);
    CHECK(ret3.value() == p.name);
    CHECK(ret4.value() == p.name);
  }
}

TEST_CASE("testing pack object") {
  person p{20, "tom"};
  auto ret = serialize(p);

  person p1{};
  auto ec = deserialize_to(p1, ret.data(), ret.size());
  CHECK(ec == std::errc{});
  CHECK(p == p1);

  CHECK(deserialize_to(p1, ret.data(), 4) == std::errc::no_buffer_space);

  SUBCASE("test empty string") {
    p.name = "";
    ret = serialize(p);
    person p2{};
    ec = deserialize_to(p2, ret.data(), ret.size());
    CHECK(ec == std::errc{});
    CHECK(p == p2);
  }
}

TEST_CASE("test buffer size") {
  person p{20, "tom"};
  auto size = get_needed_size(p);

  SUBCASE("test great than need") {
    auto buffer_size = size + 100;
    auto buffer = std::make_unique<char[]>(buffer_size);
    auto ret = serialize_to(buffer.get(), buffer_size, p);
    CHECK(ret == size);
  }

  SUBCASE("test less than need") {
    auto buffer_size = size - 1;
    auto buffer = std::make_unique<char[]>(buffer_size);
    auto ret = serialize_to(buffer.get(), buffer_size, p);
    CHECK(ret == 0);
  }
}
void test_container(auto &v) {
  auto ret = serialize(v);

  using T = std::remove_cvref_t<decltype(v)>;
  T v1{};
  auto ec = deserialize_to(v1, ret.data(), ret.size());
  CHECK(ec == std::errc{});
  CHECK(v == v1);

  v.clear();
  v1.clear();
  ret = serialize(v);
  ec = deserialize_to(v1, ret.data(), ret.size());
  CHECK(ec == std::errc{});
  CHECK(v1.empty());
  CHECK(v == v1);
}

TEST_CASE("testing long long") {
  if constexpr (sizeof(long long) == 8) {
    auto ret = serialize(-1ll);
    auto res = deserialize<long long>(ret.data(), ret.size());
    CHECK(res);
    CHECK(res.value() == -1ll);
  }
  if constexpr (sizeof(unsigned long long) == 8) {
    auto ret = serialize(1ull);
    auto res = deserialize<unsigned long long>(ret.data(), ret.size());
    CHECK(res);
    CHECK(res.value() == 1ull);
  }
}

TEST_CASE("testing sequence containers") {
  SUBCASE("test vector") {
    std::vector<person> v{{20, "tom"}};
    test_container(v);
  }
  SUBCASE("test list") {
    std::list<person> v{{20, "tom"}};
    test_container(v);
  }
  SUBCASE("test deque") {
    std::deque<person> v{{20, "tom"}};
    test_container(v);
  }
}

TEST_CASE("testing associative containers") {
  SUBCASE("test int map") {
    std::map<int, person> v{{1, {20, "tom"}}, {2, {22, "jerry"}}};
    test_container(v);
  }

  SUBCASE("test string map") {
    std::map<std::string, person> v{{"aa", {20, "tom"}}, {"bb", {22, "jerry"}}};
    test_container(v);
  }

  SUBCASE("test multimap") {
    std::multimap<int, person> v{{1, {20, "tom"}}, {2, {22, "jerry"}}};
    test_container(v);

    std::multimap<int, person> v1{{1, {20, "tom"}}, {1, {22, "jerry"}}};
    test_container(v1);

    std::multimap<int, person> v2{
        {1, {20, "tom"}}, {1, {22, "jerry"}}, {3, {22, "jack"}}};
    test_container(v2);

    std::multimap<std::string, person> v3{{"aa", {20, "tom"}},
                                          {"bb", {22, "jerry"}}};
    test_container(v3);

    std::multimap<std::string, person> v4{
        {"dd", {20, "tom"}}, {"aa", {22, "jerry"}}, {"aa", {20, "jack"}}};
    test_container(v4);
  }

  SUBCASE("test set") {
    std::set<int> v{1, 2};
    test_container(v);

    std::set<std::string> v2{"aa", "bb"};
    test_container(v2);

    std::set<person> v3{{20, "tom"}, {22, "jerry"}};
    test_container(v3);
  }

  SUBCASE("test multiset") {
    std::multiset<int> v{1, 2, 1, 2};
    test_container(v);

    std::multiset<std::string> v2{"aa", "bb", "aa"};
    test_container(v2);

    std::multiset<person> v3{{20, "tom"}, {22, "jerry"}, {20, "jack"}};
    test_container(v3);
  }
}

TEST_CASE("testing unordered associative containers") {
  SUBCASE("test int map") {
    std::unordered_map<int, person> v{{1, {20, "tom"}}, {2, {22, "jerry"}}};
    test_container(v);
  }

  SUBCASE("test string map") {
    std::unordered_map<std::string, person> v{{"aa", {20, "tom"}},
                                              {"bb", {22, "jerry"}}};
    test_container(v);
  }

  SUBCASE("test multimap") {
    std::unordered_map<int, person> v{{1, {20, "tom"}}, {2, {22, "jerry"}}};
    test_container(v);

    std::unordered_map<int, person> v1{{1, {20, "tom"}}, {1, {22, "jerry"}}};
    test_container(v1);

    std::unordered_map<int, person> v2{
        {1, {20, "tom"}}, {1, {22, "jerry"}}, {3, {22, "jack"}}};
    test_container(v2);

    std::unordered_map<std::string, person> v3{{"aa", {20, "tom"}},
                                               {"bb", {22, "jerry"}}};
    test_container(v3);

    std::unordered_map<std::string, person> v4{
        {"dd", {20, "tom"}}, {"aa", {22, "jerry"}}, {"aa", {20, "jack"}}};
    test_container(v4);
  }
}

TEST_CASE("testing don't support container adaptors") {
  std::stack<int> s;
  s.push(1);
  s.push(2);

  //  struct_pack::packer o{};
  //  o.serialize(s); //will compile error
}

// We should not inherit from stl container, this case just for testing.
template <typename T>
struct my_vector : public std::vector<T> {};

template <typename Key, typename Value>
struct my_map : public std::map<Key, Value> {};

TEST_CASE("testing nonstd containers") {
  SUBCASE("test custom vector") {
    my_vector<int> v;
    v.push_back(1);
    v.push_back(2);
    test_container(v);

    my_vector<std::string> v1;
    v1.push_back("aa");
    v1.push_back("bb");
    test_container(v1);

    my_vector<person> v2;
    v2.push_back({20, "tom"});
    test_container(v2);
  }

  SUBCASE("test custom map") {
    my_map<int, person> v;
    v.emplace(1, person{20, "tom"});
    v.emplace(2, person{22, "jerry"});
    test_container(v);
  }
}

void test_tuple_like(auto &v) {
  auto ret = serialize(v);

  using T = std::remove_cvref_t<decltype(v)>;
  T v1{};
  auto ec = deserialize_to(v1, ret.data(), ret.size());
  CHECK(ec == std::errc{});
  CHECK(v == v1);
}

TEST_CASE("testing tuple") {
  std::tuple<int, std::string> v = std::make_tuple(1, "hello");
  test_tuple_like(v);

  std::tuple<int, std::string> v1{};
  test_tuple_like(v1);

  std::tuple<int, std::string, person> v2{1, "aa", {20, "tom"}};
  test_tuple_like(v2);
}

TEST_CASE("test std::pair") {
  std::pair<int, std::string> v{1, "hello"};
  test_tuple_like(v);

  std::pair<std::string, person> v1{"aa", {20, "tom"}};
  test_tuple_like(v1);

  std::pair<std::string, person> v2{};
  test_tuple_like(v2);
}

TEST_CASE("testing std::array") {
  std::array<int, 3> v{1, 2, 3};
  test_tuple_like(v);

  std::array<std::string, 2> v1{"tom", "jerry"};
  test_tuple_like(v1);

  std::array<person, 2> v2{person{20, "tom"}, {22, "jerry"}};
  test_tuple_like(v2);

  std::array<person, 2> v3{};
  test_tuple_like(v3);
}

TEST_CASE("test_trivial_copy_tuple") {
  tuplet::tuple tp = tuplet::make_tuple(1, 2);

  constexpr auto count = detail::member_count<decltype(tp)>();
  static_assert(count == 2);

  static_assert(std::is_same_v<decltype(tp), tuplet::tuple<int, int>>);
  static_assert(!std::is_same_v<decltype(tp), std::tuple<int, int>>);

  static_assert(
      std::is_same_v<decltype(detail::get_types(tp)), tuplet::tuple<int, int>>);
  static_assert(
      !std::is_same_v<decltype(detail::get_types(tp)), std::tuple<int, int>>);

  static_assert(get_type_code<decltype(tp)>() !=
                get_type_code<std::tuple<int, int>>());

  auto buf = serialize(tp);

  std::tuple<int, int> v{};
  auto ec = deserialize_to(v, buf);
  CHECK(ec != std::errc{});

  decltype(tp) tp1;
  auto ec2 = deserialize_to(tp1, buf);
  CHECK(ec2 == std::errc{});
  CHECK(tp == tp1);
}

struct test_obj {
  int id;
  std::string str;
  tuplet::tuple<int, std::string> tp;
  int d;
};

TEST_CASE("test_trivial_copy_tuple in an object") {
  test_obj obj{1, "hello", {2, "tuple"}, 3};
  auto buf = serialize(obj);

  test_obj obj1;
  auto ec = deserialize_to(obj1, buf);
  CHECK(ec == std::errc{});
  CHECK(obj.tp == obj1.tp);
}

void test_c_array(auto &v) {
  auto ret = serialize(v);

  using T = std::remove_cvref_t<decltype(v)>;
  T v1{};
  auto ec = deserialize_to(v1, ret.data(), ret.size());
  REQUIRE(ec == std::errc{});

  auto size = std::extent_v<T>;
  for (int i = 0; i < size; ++i) {
    CHECK(v[i] == v1[i]);
  }
}

TEST_CASE("testing c array") {
  int v[3] = {1, 2, 3};
  test_c_array(v);
  int v1[2] = {};
  test_c_array(v1);

  std::string v3[2] = {"hello", "world"};
  test_c_array(v3);

  person v4[2] = {{20, "tom"}, {22, "jerry"}};
  test_c_array(v4);
}

enum class enum_i8 : int8_t { hello, hi };
enum class enum_i32 : int32_t { hello, hi };

TEST_CASE("testing enum") {
  {
    enum_i8 e{enum_i8::hi}, e2;
    auto ret = serialize(e);
    std::size_t sz;
    auto ec = deserialize_to(e2, ret.data(), ret.size(), sz);
    CHECK(ec == std::errc{});
    CHECK(sz == ret.size());
    CHECK(e == e2);
  }
  {
    enum_i8 e{enum_i8::hi};
    auto ret = serialize(e);
    auto ec = deserialize<enum_i32>(ret.data(), ret.size());
    CHECK(!ec);
    if (!ec) {
      CHECK(ec.error() == std::errc::invalid_argument);
    }
  }
  {
    constexpr auto code_enum_i8 = get_type_code<enum_i8>();
    constexpr auto code_enum_i32 = get_type_code<enum_i32>();
    constexpr auto code_i8 = get_type_code<int8_t>();
    constexpr auto code_i32 = get_type_code<int32_t>();
    static_assert(code_enum_i8 != code_enum_i32);
    static_assert(code_enum_i8 == code_i8);
    static_assert(code_enum_i32 == code_i32);
  }
}

TEST_CASE("testing fundamental types") {
  {
    std::array ar = {get_type_code<int8_t>(),   get_type_code<int16_t>(),
                     get_type_code<int32_t>(),  get_type_code<int64_t>(),
                     get_type_code<uint8_t>(),  get_type_code<uint16_t>(),
                     get_type_code<uint32_t>(), get_type_code<uint64_t>(),
                     get_type_code<char>(),     get_type_code<wchar_t>(),
                     get_type_code<char16_t>(), get_type_code<char32_t>(),
                     get_type_code<float>(),    get_type_code<double>()};
    std::sort(ar.begin(), ar.end());
    CHECK(std::unique(ar.begin(), ar.end()) == ar.end());
  }
  {
    static_assert(get_type_literal<char>() == get_type_literal<char8_t>());
    static_assert(get_type_literal<signed char>() ==
                  get_type_literal<int8_t>());
    static_assert(get_type_literal<unsigned char>() ==
                  get_type_literal<uint8_t>());
  }
}

TEST_CASE("test variant") {
  {
    constexpr auto variant_i = get_type_code<std::variant<int>>();
    constexpr auto variant_i_f = get_type_code<std::variant<int, float>>();
    constexpr auto i = get_type_code<int>();
    constexpr auto tuple_i = get_type_code<std::tuple<int>>();
    static_assert(variant_i != variant_i_f);
    static_assert(variant_i != i);
    static_assert(variant_i != tuple_i);
  }
  {
    std::variant<int, double> var = 1.4, var2;
    auto ret = struct_pack::serialize(var);
    auto ec = struct_pack::deserialize_to(var2, ret.data(), ret.size());
    CHECK(ec == std::errc{});
    CHECK(var2 == var);
  }
  {
    std::variant<int, std::monostate> var = std::monostate{}, var2;
    auto ret = struct_pack::serialize(var);
    auto ec = struct_pack::deserialize_to(var2, ret.data(), ret.size());
    CHECK(ec == std::errc{});
    CHECK(var2 == var);
  }
  {
    std::variant<int, int, int, std::monostate, double, double, double> var{
        std::in_place_index_t<1>{}, 2},
        var2;
    auto ret = struct_pack::serialize(var);
    auto ec = struct_pack::deserialize_to(var2, ret.data(), ret.size());
    CHECK(ec == std::errc{});
    CHECK(var2 == var);
    CHECK(var2.index() == 1);
  }
  {
    std::variant<std::monostate, std::string> var{"hello"}, var2;
    auto ret = struct_pack::serialize(var);
    auto ec = struct_pack::deserialize_to(var2, ret.data(), ret.size());
    CHECK(ec == std::errc{});
    CHECK(var2 == var);
  }
  {
    std::tuple<int, std::variant<int, double>, double,
               std::variant<int, double>, double>
        var = {1, 2.0, 3.0, 42, 5.0};
    auto ret = struct_pack::serialize(var);
    auto res = struct_pack::get_field<decltype(var), 3>(ret.data(), ret.size());
    CHECK(res);
    CHECK(res.value() == std::variant<int, double>(42));
  }
  { auto ret = struct_pack::serialize(std::tuple<std::monostate>{}); }
  {
    std::variant<
        int8_t, int8_t, int8_t, int8_t, int8_t, int8_t, int8_t, int8_t, int8_t,
        int8_t, int16_t, int16_t, int16_t, int16_t, int16_t, int16_t, int16_t,
        int16_t, int16_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t,
        int32_t, int32_t, int32_t, int64_t, int64_t, int64_t, int64_t, int64_t,
        int64_t, int64_t, int64_t, int64_t, std::string, std::monostate,
        std::unordered_map<int, std::variant<std::monostate, double, int>>>
        big_variant =
            {std::unordered_map<int, std::variant<std::monostate, double, int>>{
                {1, 1}, {2, 0.2}, {-1, std::monostate{}}}},
        big_variant2;
    auto ret = struct_pack::serialize(big_variant);
    auto ec = struct_pack::deserialize_to(big_variant2, ret.data(), ret.size());
    CHECK(ec == std::errc{});
    CHECK(big_variant2 == big_variant);
  }
}

TEST_CASE("test monostate") {
  {
    std::monostate var, var2;
    static_assert(struct_pack::get_needed_size(std::monostate{}) == 4);
    auto ret = struct_pack::serialize(var);
    auto ec = struct_pack::deserialize_to(var2, ret.data(), ret.size());
    CHECK(ec == std::errc{});
    CHECK(var2 == var);
  }
}

TEST_CASE("test expected") {
  {
    tl::expected<int, std::errc> exp{42}, exp2;
    auto ret = serialize(exp);
    auto res = deserialize_to(exp2, ret.data(), ret.size());
    CHECK(res == std::errc{});
    CHECK(exp2 == exp);
  }
  {
    tl::expected<std::vector<int>, std::errc> exp{std::vector{41, 42, 43}},
        exp2;
    auto ret = serialize(exp);
    auto res = deserialize_to(exp2, ret.data(), ret.size());
    CHECK(res == std::errc{});
    CHECK(exp2 == exp);
  }
  {
    tl::expected<std::vector<int>, std::errc> exp{
        tl::unexpected{std::errc::address_in_use}},
        exp2;

    auto ret = serialize(exp);
    auto res = deserialize_to(exp2, ret.data(), ret.size());
    CHECK(res == std::errc{});
    CHECK(exp2 == exp);
  }
}

TEST_CASE("testing object with containers, enum, tuple array, and pair") {
  complicated_object v{
      .color = Color::red,
      .a = 42,
      .b = "hello",
      .c = {{20, "tom"}, {22, "jerry"}},
      .d = {"hello", "world"},
      .e = {1, 2},
      .f = {{1, {20, "tom"}}},
      .g = {{1, {20, "tom"}}, {1, {22, "jerry"}}},
      .h = {"aa", "bb"},
      .i = {1, 2},
      .j = {{1, {20, "tom"}}, {1, {22, "jerry"}}},
      .k = {{1, 2}, {1, 3}},
      .m = {person{20, "tom"}, {22, "jerry"}},
      .n = {person{20, "tom"}, {22, "jerry"}},
      .o = std::make_pair("aa", person{20, "tom"}),
  };
  auto ret = serialize(v);

  complicated_object v1{};
  auto ec = deserialize_to(v1, ret.data(), ret.size());
  CHECK(ec == std::errc{});
  CHECK(v.g == v1.g);
  CHECK(v.h == v1.h);
  CHECK(v.i == v1.i);
  CHECK(v.j == v1.j);
  CHECK(v.k == v1.k);
  CHECK(v.m == v1.m);
  CHECK(v.o == v1.o);

  SUBCASE("test nested object") {
    nested_object nested{.id = 2, .name = "tom", .p = {20, "tom"}, .o = v1};
    ret = serialize(nested);

    nested_object nested1{};
    auto ec = deserialize_to(nested1, ret.data(), ret.size());
    CHECK(ec == std::errc{});
    CHECK(nested.p == nested1.p);
    CHECK(nested.name == nested1.name);
    CHECK(nested.o.m == nested1.o.m);
    CHECK(nested.o.o == nested1.o.o);
  }

  SUBCASE("test get_field") {
    auto pair = get_field<complicated_object, 2>(ret.data(), ret.size());
    CHECK(pair);
    CHECK(pair.value() == "hello");
    pair = get_field<complicated_object, 2>(ret);
    CHECK(pair);
    CHECK(pair.value() == "hello");
    auto pair1 = get_field<complicated_object, 14>(ret.data(), ret.size());
    CHECK(pair1);
    CHECK(pair1.value() == v.o);
    pair1 = get_field<complicated_object, 14>(ret);
    CHECK(pair1);
    CHECK(pair1.value() == v.o);

    auto res = get_field<complicated_object, 14>(ret.data(), 24);
    CHECK(!res);
    if (!res) {
      CHECK(res.error() == std::errc::no_buffer_space);
    }
  }
  SUBCASE("test get_field_to") {
    std::string res1;
    auto ec = get_field_to<complicated_object, 2>(res1, ret.data(), ret.size());
    CHECK(ec == std::errc{});
    CHECK(res1 == "hello");
    ec = get_field_to<complicated_object, 2>(res1, ret);
    CHECK(ec == std::errc{});
    CHECK(res1 == "hello");
    std::pair<std::string, person> res2;
    ec = get_field_to<complicated_object, 14>(res2, ret.data(), ret.size());
    CHECK(ec == std::errc{});
    CHECK(res2 == v.o);
    ec = get_field_to<complicated_object, 14>(res2, ret);
    CHECK(ec == std::errc{});
    CHECK(res2 == v.o);

    auto res = get_field_to<complicated_object, 14>(res2, ret.data(), 24);
    CHECK(ec == std::errc{});
    if (ec != std::errc{}) {
      CHECK(ec == std::errc::no_buffer_space);
    }
  }
}

TEST_CASE("testing exceptions") {
  std::string buffer;
  buffer.resize(2);
  auto ret = serialize_to(buffer.data(), buffer.size(), std::string("hello"));
  CHECK(ret == 0);

  std::array<std::string, 2> arr{"hello", "struct_pack"};
  auto size = get_needed_size(arr);
  buffer.resize(size);
  ret = serialize_to(buffer.data(), size - 1, arr);
  CHECK(ret == 0);

  std::pair<int, std::string> pair{2, "hello"};
  size = get_needed_size(pair);
  buffer.resize(size);
  ret = serialize_to(buffer.data(), size - 1, pair);
  CHECK(ret == 0);

  std::map<int, std::string> map{{1, "hello"}};
  size = get_needed_size(pair);
  buffer.resize(size);
  ret = serialize_to(buffer.data(), size - 1, map);
  CHECK(ret == 0);
  ret = serialize_to(buffer.data(), size - 6, map);
  CHECK(ret == 0);
  ret = serialize_to(buffer.data(), size - 10, map);
  CHECK(ret == 0);

  std::vector<int> data_list = {1, 2, 3};
  std::vector<std::byte> my_byte_buffer;
  serialize_to(my_byte_buffer, data_list);
  std::vector<int> data_list2;
  auto ec = deserialize_to(data_list2, my_byte_buffer.data(), 0);
  CHECK(ec == std::errc::no_buffer_space);
  ec = deserialize_to(data_list2, my_byte_buffer.data(),
                      my_byte_buffer.size() - 1);
  CHECK(ec == std::errc::no_buffer_space);

  std::vector<std::byte> my_byte_buffer2;
  std::optional<bool> bool_opt;
  serialize_to(my_byte_buffer2, bool_opt);
  auto ret3 = deserialize<std::optional<bool>>(my_byte_buffer2.data(),
                                               my_byte_buffer2.size() - 1);
  CHECK(!ret3);
  if (!ret3) {
    CHECK(ret3.error() == std::errc::no_buffer_space);
  }
}

TEST_CASE("testing serialize/deserialize varadic params") {
  {
    auto ret = struct_pack::serialize(1, 2, 3, 4, 5);
    auto res = struct_pack::deserialize<std::tuple<int, int, int, int, int>>(
        ret.data(), ret.size());
    CHECK(res);
    CHECK(std::get<0>(res.value()) == 1);
    CHECK(std::get<1>(res.value()) == 2);
    CHECK(std::get<2>(res.value()) == 3);
    CHECK(std::get<3>(res.value()) == 4);
    CHECK(std::get<4>(res.value()) == 5);
  }
  {
    auto ret = struct_pack::serialize(1, 2, 3, 4, 5);
    auto res =
        struct_pack::deserialize<std::tuple<int, int, int, int, int>>(ret);
    CHECK(res);
    CHECK(std::get<0>(res.value()) == 1);
    CHECK(std::get<1>(res.value()) == 2);
    CHECK(std::get<2>(res.value()) == 3);
    CHECK(std::get<3>(res.value()) == 4);
    CHECK(std::get<4>(res.value()) == 5);
  }
  {
    auto ret = struct_pack::serialize(1, 2, 3, 4, 5);
    auto res = struct_pack::deserialize<int, int, int, int, int>(ret.data(),
                                                                 ret.size());
    CHECK(res);
    CHECK(std::get<0>(res.value()) == 1);
    CHECK(std::get<1>(res.value()) == 2);
    CHECK(std::get<2>(res.value()) == 3);
    CHECK(std::get<3>(res.value()) == 4);
    CHECK(std::get<4>(res.value()) == 5);
  }
  {
    auto ret = struct_pack::serialize(1, 2, 3, 4, 5);
    auto res = struct_pack::deserialize<int, int, int, int, int>(ret);
    CHECK(res);
    CHECK(std::get<0>(res.value()) == 1);
    CHECK(std::get<1>(res.value()) == 2);
    CHECK(std::get<2>(res.value()) == 3);
    CHECK(std::get<3>(res.value()) == 4);
    CHECK(std::get<4>(res.value()) == 5);
  }
  {
    std::vector<char> ret;
    struct_pack::serialize_to(ret, 1, 2, 3, 4, 5);
    auto res = struct_pack::deserialize<int, int, int, int, int>(ret.data(),
                                                                 ret.size());
    CHECK(res);
    auto &values = res.value();
    CHECK(std::get<0>(values) == 1);
    CHECK(std::get<1>(values) == 2);
    CHECK(std::get<2>(values) == 3);
    CHECK(std::get<3>(values) == 4);
    CHECK(std::get<4>(values) == 5);
  }
  {
    std::vector<char> ret;
    struct_pack::serialize_to(ret, 1, 2, 3, 4, 5);
    auto res = struct_pack::deserialize<int, int, int, int, int>(ret);
    CHECK(res);
    auto &values = res.value();
    CHECK(std::get<0>(values) == 1);
    CHECK(std::get<1>(values) == 2);
    CHECK(std::get<2>(values) == 3);
    CHECK(std::get<3>(values) == 4);
    CHECK(std::get<4>(values) == 5);
  }
  {
    auto ret = struct_pack::serialize(1, 2, 3, 4, 5);
    int a[5];
    auto res = struct_pack::deserialize_to(a[0], ret.data(), ret.size(), a[1],
                                           a[2], a[3], a[4]);
    CHECK(res == std::errc{});
    CHECK(a[0] == 1);
    CHECK(a[1] == 2);
    CHECK(a[2] == 3);
    CHECK(a[3] == 4);
    CHECK(a[4] == 5);
  }
  {
    auto ret = struct_pack::serialize(1, 2, 3, 4, 5);
    int a[5];
    auto res = struct_pack::deserialize_to(a[0], ret, a[1], a[2], a[3], a[4]);
    CHECK(res == std::errc{});
    CHECK(a[0] == 1);
    CHECK(a[1] == 2);
    CHECK(a[2] == 3);
    CHECK(a[3] == 4);
    CHECK(a[4] == 5);
  }
  {
    auto ret = struct_pack::serialize(
        42, 2.71828f, 3.1415926, 71086291, 1145141919810Ull,
        std::string{"Hello"},
        std::array<std::vector<int>, 3>{
            {{1, 1, 4, 5, 1, 4}, {1919, 810}, {710862, 91}}},
        std::variant<int, float, double>{1.4});
    auto res = struct_pack::deserialize<std::tuple<
        int, float, double, int, unsigned long long, std::string,
        std::array<std::vector<int>, 3>, std::variant<int, float, double>>>(
        ret.data(), ret.size());
    CHECK(res);
    auto &values = res.value();
    CHECK(std::get<0>(values) == 42);
    CHECK(std::get<1>(values) == 2.71828f);
    CHECK(std::get<2>(values) == 3.1415926);
    CHECK(std::get<3>(values) == 71086291);
    CHECK(std::get<4>(values) == 1145141919810Ull);
    CHECK(std::get<5>(values) == std::string{"Hello"});
    CHECK(std::get<6>(values) ==
          std::array<std::vector<int>, 3>{
              {{1, 1, 4, 5, 1, 4}, {1919, 810}, {710862, 91}}});
    CHECK(std::get<7>(values) == std::variant<int, float, double>{1.4});
  }
  {
    auto ret = struct_pack::serialize(
        42, 2.71828f, 3.1415926, 71086291, 1145141919810Ull,
        std::string{"Hello"},
        std::array<std::vector<int>, 3>{
            {{1, 1, 4, 5, 1, 4}, {1919, 810}, {710862, 91}}},
        std::variant<int, float, double>{1.4});
    auto res = struct_pack::deserialize<std::tuple<
        int, float, double, int, unsigned long long, std::string,
        std::array<std::vector<int>, 3>, std::variant<int, float, double>>>(
        ret);
    CHECK(res);
    auto &values = res.value();
    CHECK(std::get<0>(values) == 42);
    CHECK(std::get<1>(values) == 2.71828f);
    CHECK(std::get<2>(values) == 3.1415926);
    CHECK(std::get<3>(values) == 71086291);
    CHECK(std::get<4>(values) == 1145141919810Ull);
    CHECK(std::get<5>(values) == std::string{"Hello"});
    CHECK(std::get<6>(values) ==
          std::array<std::vector<int>, 3>{
              {{1, 1, 4, 5, 1, 4}, {1919, 810}, {710862, 91}}});
    CHECK(std::get<7>(values) == std::variant<int, float, double>{1.4});
  }
  {
    auto ret = struct_pack::serialize(
        42, 2.71828f, 3.1415926, 71086291, 1145141919810Ull,
        std::string{"Hello"},
        std::array<std::vector<int>, 3>{
            {{1, 1, 4, 5, 1, 4}, {1919, 810}, {710862, 91}}},
        std::variant<int, float, double>{1.4});
    auto res =
        struct_pack::deserialize<int, float, double, int, unsigned long long,
                                 std::string, std::array<std::vector<int>, 3>,
                                 std::variant<int, float, double>>(ret.data(),
                                                                   ret.size());
    CHECK(res);
    auto &values = res.value();
    CHECK(std::get<0>(values) == 42);
    CHECK(std::get<1>(values) == 2.71828f);
    CHECK(std::get<2>(values) == 3.1415926);
    CHECK(std::get<3>(values) == 71086291);
    CHECK(std::get<4>(values) == 1145141919810Ull);
    CHECK(std::get<5>(values) == std::string{"Hello"});
    CHECK(std::get<6>(values) ==
          std::array<std::vector<int>, 3>{
              {{1, 1, 4, 5, 1, 4}, {1919, 810}, {710862, 91}}});
    CHECK(std::get<7>(values) == std::variant<int, float, double>{1.4});
  }
  {
    auto ret = struct_pack::serialize(
        42, 2.71828f, 3.1415926, 71086291, 1145141919810Ull,
        std::string{"Hello"},
        std::array<std::vector<int>, 3>{
            {{1, 1, 4, 5, 1, 4}, {1919, 810}, {710862, 91}}},
        std::variant<int, float, double>{1.4});
    auto res =
        struct_pack::deserialize<int, float, double, int, unsigned long long,
                                 std::string, std::array<std::vector<int>, 3>,
                                 std::variant<int, float, double>>(ret);
    CHECK(res);
    auto &values = res.value();
    CHECK(std::get<0>(values) == 42);
    CHECK(std::get<1>(values) == 2.71828f);
    CHECK(std::get<2>(values) == 3.1415926);
    CHECK(std::get<3>(values) == 71086291);
    CHECK(std::get<4>(values) == 1145141919810Ull);
    CHECK(std::get<5>(values) == std::string{"Hello"});
    CHECK(std::get<6>(values) ==
          std::array<std::vector<int>, 3>{
              {{1, 1, 4, 5, 1, 4}, {1919, 810}, {710862, 91}}});
    CHECK(std::get<7>(values) == std::variant<int, float, double>{1.4});
  }
  {
    std::vector<char> ret;
    struct_pack::serialize_to(
        ret, 42, 2.71828f, 3.1415926, 71086291, 1145141919810Ull,
        std::string{"Hello"},
        std::array<std::vector<int>, 3>{
            {{1, 1, 4, 5, 1, 4}, {1919, 810}, {710862, 91}}},
        std::variant<int, float, double>{1.4});
    auto res =
        struct_pack::deserialize<int, float, double, int, unsigned long long,
                                 std::string, std::array<std::vector<int>, 3>,
                                 std::variant<int, float, double>>(ret.data(),
                                                                   ret.size());
    CHECK(res);
    auto &values = res.value();
    CHECK(std::get<0>(values) == 42);
    CHECK(std::get<1>(values) == 2.71828f);
    CHECK(std::get<2>(values) == 3.1415926);
    CHECK(std::get<3>(values) == 71086291);
    CHECK(std::get<4>(values) == 1145141919810Ull);
    CHECK(std::get<5>(values) == std::string{"Hello"});
    CHECK(std::get<6>(values) ==
          std::array<std::vector<int>, 3>{
              {{1, 1, 4, 5, 1, 4}, {1919, 810}, {710862, 91}}});
    CHECK(std::get<7>(values) == std::variant<int, float, double>{1.4});
  }
  {
    std::vector<char> ret;
    struct_pack::serialize_to(
        ret, 42, 2.71828f, 3.1415926, 71086291, 1145141919810Ull,
        std::string{"Hello"},
        std::array<std::vector<int>, 3>{
            {{1, 1, 4, 5, 1, 4}, {1919, 810}, {710862, 91}}},
        std::variant<int, float, double>{1.4});
    auto res =
        struct_pack::deserialize<int, float, double, int, unsigned long long,
                                 std::string, std::array<std::vector<int>, 3>,
                                 std::variant<int, float, double>>(ret);
    CHECK(res);
    auto &values = res.value();
    CHECK(std::get<0>(values) == 42);
    CHECK(std::get<1>(values) == 2.71828f);
    CHECK(std::get<2>(values) == 3.1415926);
    CHECK(std::get<3>(values) == 71086291);
    CHECK(std::get<4>(values) == 1145141919810Ull);
    CHECK(std::get<5>(values) == std::string{"Hello"});
    CHECK(std::get<6>(values) ==
          std::array<std::vector<int>, 3>{
              {{1, 1, 4, 5, 1, 4}, {1919, 810}, {710862, 91}}});
    CHECK(std::get<7>(values) == std::variant<int, float, double>{1.4});
  }
  {
    std::vector<char> ret;
    struct_pack::serialize_to(
        ret, 42, 2.71828f, 3.1415926, 71086291, 1145141919810Ull,
        std::string{"Hello"},
        std::array<std::vector<int>, 3>{
            {{1, 1, 4, 5, 1, 4}, {1919, 810}, {710862, 91}}},
        std::variant<int, float, double>{1.4});
    std::tuple<int, float, double, int, unsigned long long, std::string,
               std::array<std::vector<int>, 3>,
               std::variant<int, float, double>>
        t;
    auto res = struct_pack::deserialize_to(
        std::get<0>(t), ret.data(), ret.size(), std::get<1>(t), std::get<2>(t),
        std::get<3>(t), std::get<4>(t), std::get<5>(t), std::get<6>(t),
        std::get<7>(t));
    CHECK(res == std::errc{});
    CHECK(std::get<0>(t) == 42);
    CHECK(std::get<1>(t) == 2.71828f);
    CHECK(std::get<2>(t) == 3.1415926);
    CHECK(std::get<3>(t) == 71086291);
    CHECK(std::get<4>(t) == 1145141919810Ull);
    CHECK(std::get<5>(t) == std::string{"Hello"});
    CHECK(std::get<6>(t) ==
          std::array<std::vector<int>, 3>{
              {{1, 1, 4, 5, 1, 4}, {1919, 810}, {710862, 91}}});
    CHECK(std::get<7>(t) == std::variant<int, float, double>{1.4});
  }
  {
    std::vector<char> ret;
    struct_pack::serialize_to(
        ret, 42, 2.71828f, 3.1415926, 71086291, 1145141919810Ull,
        std::string{"Hello"},
        std::array<std::vector<int>, 3>{
            {{1, 1, 4, 5, 1, 4}, {1919, 810}, {710862, 91}}},
        std::variant<int, float, double>{1.4});
    std::tuple<int, float, double, int, unsigned long long, std::string,
               std::array<std::vector<int>, 3>,
               std::variant<int, float, double>>
        t;
    auto res = struct_pack::deserialize_to(
        std::get<0>(t), ret, std::get<1>(t), std::get<2>(t), std::get<3>(t),
        std::get<4>(t), std::get<5>(t), std::get<6>(t), std::get<7>(t));
    CHECK(res == std::errc{});
    CHECK(std::get<0>(t) == 42);
    CHECK(std::get<1>(t) == 2.71828f);
    CHECK(std::get<2>(t) == 3.1415926);
    CHECK(std::get<3>(t) == 71086291);
    CHECK(std::get<4>(t) == 1145141919810Ull);
    CHECK(std::get<5>(t) == std::string{"Hello"});
    CHECK(std::get<6>(t) ==
          std::array<std::vector<int>, 3>{
              {{1, 1, 4, 5, 1, 4}, {1919, 810}, {710862, 91}}});
    CHECK(std::get<7>(t) == std::variant<int, float, double>{1.4});
  }
}

TEST_CASE("testing string_view deserialize") {
  using namespace std;
  {
    auto ret = serialize("hello"sv);
    auto res = deserialize<string_view>(ret.data(), ret.size());
    CHECK(res);
    CHECK(res.value() == "hello"sv);
  }
  {
    std::u32string_view sv = U"你好";
    auto ret = serialize(sv);
    auto res = deserialize<std::u32string_view>(ret.data(), ret.size());
    CHECK(res);
    CHECK(res.value() == sv);
  }
  {
    std::u16string_view sv = u"你好";
    auto ret = serialize(sv);
    auto res = deserialize<std::u16string_view>(ret.data(), ret.size());
    CHECK(res);
    CHECK(res.value() == sv);
  }
  {
    std::u8string_view sv = u8"你好";
    auto ret = serialize(sv);
    auto res = deserialize<std::u8string_view>(ret.data(), ret.size());
    CHECK(res);
    CHECK(res.value() == sv);
  }
  {
    auto ret = serialize("hello"s);
    auto res = deserialize<string_view>(ret.data(), ret.size());
    CHECK(res);
    CHECK(res.value() == "hello"sv);
  }
  {
    std::u32string sv = U"你好";
    auto ret = serialize(sv);
    auto res = deserialize<std::u32string_view>(ret.data(), ret.size());
    CHECK(res);
    CHECK(res.value() == sv);
  }
  {
    std::u16string sv = u"你好";
    auto ret = serialize(sv);
    auto res = deserialize<std::u16string_view>(ret.data(), ret.size());
    CHECK(res);
    CHECK(res.value() == sv);
  }
  {
    std::u8string sv = u8"你好";
    auto ret = serialize(sv);
    auto res = deserialize<std::u8string_view>(ret.data(), ret.size());
    CHECK(res);
    CHECK(res.value() == sv);
  }
}

TEST_CASE("testing deserialization") {
  person p{20, "tom"};
  auto ret = serialize(p);

  person p1{};
  CHECK(deserialize_to(p1, ret.data(), ret.size()) == std::errc{});
}

TEST_CASE("testing deserialization with invalid data") {
  auto ret = serialize(-1);

  std::string str{};
  auto ret2 = deserialize_to(str, ret.data(), ret.size());
  CHECK(ret2 == std::errc::invalid_argument);

  ret2 = deserialize_to(str, ret.data(), INT32_MAX);
  CHECK(ret2 == std::errc::invalid_argument);

  SUBCASE("test invalid int") {
    ret = serialize(std::string("hello"));

    std::tuple<int, int> tp{};
    auto ret3 = deserialize_to(tp, ret.data(), ret.size());
    CHECK(ret3 == std::errc::invalid_argument);
  }
}

TEST_CASE("test get type code") {
  constexpr auto r = get_type_code<person>();
  constexpr auto r1 = get_type_code<nested_object>();
  constexpr auto r2 = get_type_code<int>();
  constexpr auto r3 = get_type_code<std::string>();
  std::set results = {r, r1, r2, r3};
  CHECK(results.size() == 4);

  SUBCASE("test get_types_code") {
    constexpr auto c = get_type_code<int, std::string>();
    constexpr auto c1 = get_type_code<int, std::string, person>();
    constexpr auto c2 = get_type_code<int, float, double>();
    constexpr auto c3 = get_type_code<int32_t, uint32_t>();
    constexpr auto c4 = get_type_code<uint32_t, int32_t>();
    constexpr auto c5 = get_type_code<int32_t>();
    constexpr auto c6 = get_type_code<uint32_t>();

    std::set set{c, c1, c2, c3, c4, c5, c6};
    CHECK(set.size() == 7);
  }

  SUBCASE("test get_types") {
    person p;
    nested_object nested;
    using namespace struct_pack::detail;
    auto t = get_types(p);
    CHECK(std::is_same_v<std::tuple<int, std::string>, decltype(t)>);

    auto t1 = get_types(std::tuple<int, std::string>{});
    CHECK(std::is_same_v<std::tuple<int, std::string>, decltype(t1)>);

    auto t2 = get_types(int(1));
    CHECK(std::is_same_v<std::tuple<int>, decltype(t2)>);

    auto t3 = get_types(std::vector<int>{});
    CHECK(std::is_same_v<std::tuple<std::vector<int>>, decltype(t3)>);

    CHECK(std::is_same_v<decltype(get_types(p)), std::tuple<int, std::string>>);
    CHECK(std::is_same_v<
          decltype(get_types(nested)),
          std::tuple<int, std::string, person, complicated_object>>);
  }

  SUBCASE("test get_type_literal") {
    auto str = get_type_literal<int, int, short>();
    CHECK(str.size() == 5);
    string_literal<char, 5> val{
        {(char)-2, 1, 1, 7, (char)-1}};  //{'1','1','7'};
    CHECK(str == val);

    auto str1 = get_type_literal<int64_t, uint64_t>();
    CHECK(str1.size() == 4);
    string_literal<char, 4> val1{{(char)-2, 3, 4, (char)-1}};  //{'3','4'};
    CHECK(str1 == val1);

    auto str2 =
        get_type_literal<int64_t, uint64_t, struct_pack::compatible<int32_t>>();
    CHECK(str2.size() == 4);
    string_literal<char, 4> val2{{(char)-2, 3, 4, (char)-1}};
    CHECK(str2 == val2);
  }
}

TEST_CASE("testing partial deserialization with index") {
  person p{20, "tom"};
  nested_object nested{};
  auto ret = serialize(p);

  auto pair = get_field<person, 1>(ret.data(), ret.size());
  CHECK(pair);
  CHECK(pair.value() == "tom");

  auto pair1 =
      get_field<std::tuple<int, std::string>, 0>(ret.data(), ret.size());
  CHECK(pair);
  CHECK(pair1.value() == 20);
}

TEST_CASE("test use wide string") {
  using namespace std;
  {
    auto sv = std::wstring(L"你好, struct pack");
    auto ret = serialize(sv);
    std::wstring str;
    auto ec = deserialize_to(str, ret.data(), ret.size());
    REQUIRE(ec == std::errc{});
    CHECK(str == sv);
  }
  {
    auto sv = std::u8string(u8"你好, struct pack");
    auto ret = serialize(sv);
    std::u8string str;
    auto ec = deserialize_to(str, ret.data(), ret.size());
    REQUIRE(ec == std::errc{});
    CHECK(str == sv);
  }
  {
    auto sv = std::u16string(u"你好, struct pack");
    auto ret = serialize(sv);
    std::u16string str;
    auto ec = deserialize_to(str, ret.data(), ret.size());
    REQUIRE(ec == std::errc{});
    CHECK(str == sv);
  }
  {
    auto sv = std::u32string(U"你好, struct pack");
    auto ret = serialize(sv);
    std::u32string str;
    auto ec = deserialize_to(str, ret.data(), ret.size());
    REQUIRE(ec == std::errc{});
    CHECK(str == sv);
  }
}

TEST_CASE("test use string_view") {
  using namespace std;
  {
    auto ret = serialize("hello struct pack"sv);
    std::string str;
    auto ec = deserialize_to(str, ret.data(), ret.size());
    REQUIRE(ec == std::errc{});
    CHECK(str == "hello struct pack"sv);
  }
  {
    auto sv = std::wstring_view(L"你好, struct pack");
    auto ret = serialize(sv);
    std::wstring str;
    auto ec = deserialize_to(str, ret.data(), ret.size());
    REQUIRE(ec == std::errc{});
    CHECK(str == sv);
  }
  {
    auto sv = std::u8string_view(u8"你好, struct pack");
    auto ret = serialize(sv);
    std::u8string str;
    auto ec = deserialize_to(str, ret.data(), ret.size());
    REQUIRE(ec == std::errc{});
    CHECK(str == sv);
  }
  {
    auto sv = std::u16string_view(u"你好, struct pack");
    auto ret = serialize(sv);
    std::u16string str;
    auto ec = deserialize_to(str, ret.data(), ret.size());
    REQUIRE(ec == std::errc{});
    CHECK(str == sv);
  }
  {
    auto sv = std::u32string_view(U"你好, struct pack");
    auto ret = serialize(sv);
    std::u32string str;
    auto ec = deserialize_to(str, ret.data(), ret.size());
    REQUIRE(ec == std::errc{});
    CHECK(str == sv);
  }
}

TEST_CASE("char test") {
  {
    char ch = '1', ch2;
    auto ret = serialize(ch);
    auto ec = deserialize_to(ch2, ret.data(), ret.size());
    REQUIRE(ec == std::errc{});
    CHECK(ch == ch2);
  }
  {
    signed char ch = '1', ch2;
    auto ret = serialize(ch);
    auto ec = deserialize_to(ch2, ret.data(), ret.size());
    REQUIRE(ec == std::errc{});
    CHECK(ch == ch2);
  }
  {
    unsigned char ch = '1', ch2;
    auto ret = serialize(ch);
    auto ec = deserialize_to(ch2, ret.data(), ret.size());
    REQUIRE(ec == std::errc{});
    CHECK(ch == ch2);
  }
  {
    wchar_t ch = L'1', ch2;
    auto ret = serialize(ch);
    auto ec = deserialize_to(ch2, ret.data(), ret.size());
    REQUIRE(ec == std::errc{});
    CHECK(ch == ch2);
  }
  {
    char8_t ch = u8'1', ch2;
    auto ret = serialize(ch);
    auto ec = deserialize_to(ch2, ret.data(), ret.size());
    REQUIRE(ec == std::errc{});
    CHECK(ch == ch2);
  }
  {
    char16_t ch = u'1', ch2;
    auto ret = serialize(ch);
    auto ec = deserialize_to(ch2, ret.data(), ret.size());
    REQUIRE(ec == std::errc{});
    CHECK(ch == ch2);
  }
  {
    char32_t ch = U'1', ch2;
    auto ret = serialize(ch);
    auto ec = deserialize_to(ch2, ret.data(), ret.size());
    REQUIRE(ec == std::errc{});
    CHECK(ch == ch2);
  }
}

template <typename T>
void test_no_buffer_space(T &t, std::vector<int> size_list) {
  auto ret = serialize(t);
  T t1{};
  for (auto size : size_list) {
    CHECK(deserialize_to(t1, ret.data(), size) == std::errc::no_buffer_space);
  }
}

TEST_CASE("test more exceptions") {
  std::map<std::string, std::string> map{{"hello", "struct pack"}};
  test_no_buffer_space(map, {4, 11});
  std::vector<std::string> vec{"hello struct pack"};
  test_no_buffer_space(vec, {2, 4, 8});
  std::set<std::string> set{"hello struct pack"};
  test_no_buffer_space(set, {0, 4, 8});
  int t = 42;
  test_no_buffer_space(t, {4});
  std::array<std::string, 1> arr{"hello struct pack"};
  test_no_buffer_space(arr, {8});
}

TEST_CASE("test get field exceptions") {
  person p{20, "tom"};
  auto ret = serialize(p);

  auto pair = get_field<int, 0>(ret.data(), ret.size());
  CHECK(!pair);
  if (!pair) {
    CHECK(pair.error() == std::errc::invalid_argument);
  }

  auto pair1 = get_field<person, 0>(ret.data(), 3);
  CHECK(!pair1);
  if (!pair1) {
    CHECK(pair1.error() == std::errc::no_buffer_space);
  }
}

TEST_CASE("test set_value") {
  person p{20, "tom"};
  auto ret = serialize(p);
  detail::unpacker in(ret.data(), ret.size());
  person p2;
  std::errc ec;
  std::string s;
  int v;
  int v2 = -1;
  auto ret1 = in.set_value<0, 0, int, int>(ec, v, std::move(v2));
  CHECK(ret1 == true);
  //
  auto ret2 = in.set_value<0, 1, int, int>(ec, v, std::move(v2));
  CHECK(ret2 == false);
}

TEST_CASE("test free functions") {
  person p{20, "tom"};
  auto buffer = serialize(p);
  CHECK(!buffer.empty());

  person p1{};
  CHECK(deserialize_to(p1, buffer.data(), buffer.size()) == std::errc{});

  std::optional<int> op1{};
  auto buf1 = serialize(op1);
  std::optional<int> op2{};
  CHECK(deserialize_to(op2, buf1.data(), buf1.size()) == std::errc{});
  CHECK(!op2.has_value());

  op1 = 42;
  auto buf2 = serialize(op1);
  CHECK(deserialize_to(op2, buf2.data(), buf2.size()) == std::errc{});
  CHECK(op2.has_value());
  CHECK(op2.value() == 42);

  CHECK(deserialize_to(op2, buf2.data(), 1) == std::errc::no_buffer_space);

  auto pair = get_field<person, 0>(buffer.data(), buffer.size());
  CHECK(pair);
  CHECK(pair.value() == 20);

  auto pair1 = get_field<person, 1>(buffer.data(), buffer.size());
  CHECK(pair1);
  CHECK(pair1.value() == "tom");
}

struct bug_member_count_struct1 {
  int i;
  struct hello {
    std::optional<int> j;
  } hi;
  double k = 3;
};

struct bug_member_count_struct2 : bug_member_count_struct1 {};

template <>
constexpr std::size_t struct_pack::members_count<bug_member_count_struct2> = 3;

TEST_CASE("test members_count") {
  {
    using t = bug_member_count_struct1;
    t b;
    auto res = struct_pack::serialize(b);
    auto ret = struct_pack::deserialize<t>(res.data(), res.size());
    CHECK(ret);
  }
  {
    using t = bug_member_count_struct2;
    t b;
    auto res = struct_pack::serialize(b);
    auto ret = struct_pack::deserialize<t>(res.data(), res.size());
    CHECK(ret);
  }
}

TEST_CASE("test compatible") {
  constexpr auto code = get_type_code<std::tuple<int, char>>();
  static_assert(code % 2 == 0);
  constexpr auto code1 =
      get_type_code<std::tuple<int, char, compatible<bool>>>();
  static_assert(code1 % 2 == 1);
  static_assert((code | 1) == (code1 | 1));
  constexpr auto code2 = get_type_code<
      std::tuple<int, char, compatible<bool>, compatible<int32_t>>>();
  static_assert((code | 1) == (code2 | 1));
  static_assert(code1 == code2);

  person1 p1{20, "tom", 1, false};
  std::vector<char> buffer;
  buffer.resize(get_needed_size(p1));

  auto ret = serialize_to(buffer.data(), buffer.size(), p1);
  CHECK(ret != 0);

  person p;
  auto res = deserialize_to(p, buffer.data(), buffer.size());
  CHECK(res == std::errc{});
  CHECK(p.name == p1.name);
  CHECK(p.age == p1.age);

  // short data
  for (int i = 0; i < buffer.size(); ++i)
    CHECK(deserialize_to(p, buffer.data(), i) == std::errc::no_buffer_space);

  // broken size
  uint64_t sz = 1000000;
  memcpy(buffer.data() + 4, &sz, sizeof(sz));
  CHECK(deserialize_to(p, buffer.data(), buffer.size()) ==
        std::errc::no_buffer_space);

  ret = serialize_to(buffer.data(), buffer.size(), p);
  CHECK(ret != 0);

  person1 p2;
  CHECK(deserialize_to(p2, buffer.data(), buffer.size()) == std::errc{});

  // CHECK(p2 == p1);
}
// c++98
template <typename T>
void f1(T &&t);

// c++20
void f1(auto t);

void f2(auto &&t);

TEST_CASE("test serialize offset") {
  uint32_t offset = 4;

  person p{20, "tom"};
  std::vector<char> buffer;
  auto need_size = get_needed_size(p);
  buffer.resize(need_size + offset);
  auto ret = serialize_to(buffer.data() + offset, buffer.size(), p);
  CHECK(ret != 0);
  person p2;
  CHECK(deserialize_to(p2, buffer.data() + offset, ret) == std::errc{});
  CHECK(p2 == p);

  std::vector<char> buffer2;
  buffer2.resize(10);
  auto original_size = buffer2.size();
  auto data_offset = original_size + offset;
  serialize_to_with_offset(buffer2, offset, p);
  CHECK(data_offset + need_size == buffer2.size());
  person p3;
  CHECK(deserialize_to(p3, buffer2.data() + data_offset, need_size) ==
        std::errc{});
  CHECK(p3 == p);
}

TEST_CASE("test de_serialize offset") {
  std::string hi = "Hello struct_pack, hi struct_pack, hey struct_pack.";
  std::vector<char> ho;
  std::vector<std::size_t> sizes;
  for (int i = 0; i < 100; ++i) {
    struct_pack::serialize_to(ho, hi + std::to_string(i));
    sizes.push_back(ho.size());
  }
  for (size_t i = 0, offset = 0; i < 100; ++i) {
    auto ret = struct_pack::deserialize_with_offset<std::string>(
        ho.data(), ho.size(), offset);
    CHECK(ret);
    CHECK(ret.value() == hi + std::to_string(i));
    CHECK(offset == sizes[i]);
  }
  for (size_t i = 0, offset = 0; i < 100; ++i) {
    auto ret = struct_pack::deserialize_with_offset<std::string>(ho, offset);
    CHECK(ret);
    CHECK(ret.value() == hi + std::to_string(i));
    CHECK(offset == sizes[i]);
  }
  for (size_t i = 0, offset = 0; i < 100; ++i) {
    std::string res;
    auto ec = struct_pack::deserialize_to_with_offset(res, ho.data(), ho.size(),
                                                      offset);
    CHECK(ec == std::errc{});
    CHECK(res == hi + std::to_string(i));
    CHECK(offset == sizes[i]);
  }
  for (size_t i = 0, offset = 0; i < 100; ++i) {
    std::string res;
    auto ec = struct_pack::deserialize_to_with_offset(res, ho, offset);
    CHECK(ec == std::errc{});
    CHECK(res == hi + std::to_string(i));
    CHECK(offset == sizes[i]);
  }
}

TEST_CASE("deque") {
  auto raw = std::deque(1e4, 1);
  auto ret = struct_pack::serialize(raw);
  std::deque<int> res;
  auto ec = struct_pack::deserialize_to(res, ret.data(), ret.size());
  CHECK(ec == std::errc{});
  CHECK(raw == res);
}
TEST_CASE("compatible convert to optional") {
  std::optional<std::string> a = "hello world";
  struct_pack::compatible<std::string> b = a;
  a = b;
  CHECK(b.value() == "hello world");
  CHECK(a.value() == "hello world");
}
struct compatible_in_nested_class {
  int a;
  struct_pack::compatible<int> c;
};

TEST_CASE("compatible not in the end of struct") {
  static_assert(
      detail::check_if_compatible_element_exist<
          0, int, struct_pack::compatible<short>, std::string>() == -1,
      "compatible member not in the end is illegal!");
  static_assert(detail::check_if_compatible_element_exist<
                    0, int, compatible_in_nested_class>() == -1,
                "compatible member in the nested class is illegal!");
  static_assert(
      detail::check_if_compatible_element_exist<0, compatible_in_nested_class,
                                                double>() == -1,
      "compatible member in the nested class is illegal!");
  static_assert(
      detail::check_if_compatible_element_exist<
          0, double, std::tuple<int, double, compatible<std::string>>>() == -1,
      "compatible member in the nested class is illegal!");
}

struct type_calculate_test_1 {
  std::tuple<std::pair<std::map<int, std::string>, std::vector<std::list<int>>>,
             std::set<int>, std::array<int64_t, 64>, std::optional<int>>
      a;
};
struct type_calculate_test_2 {
  struct {
    std::tuple<std::unordered_map<int, std::string>,
               std::deque<std::vector<int>>>
        a;
    std::multiset<int> b;
    int64_t c[64];
    std::optional<int> d;
  } e;
};

struct type_calculate_test_3 {
  struct {
    std::tuple<std::unordered_map<int, std::string>,
               std::deque<std::vector<double>>>
        a;
    std::multiset<int> b;
    int64_t c[64];
    std::optional<int> d;
  } e;
};

TEST_CASE("type calculate") {
  static_assert(std::is_trivially_copyable<struct_pack::compatible<int>>::value,
                "must be true");

  static_assert(
      get_type_code<std::vector<int>>() != get_type_code<std::vector<float>>(),
      "vector<T> with different T should get different MD5");
  static_assert(
      get_type_code<std::deque<int>>() != get_type_code<std::deque<float>>(),
      "deque<T> with different T should get different MD5");
  static_assert(
      get_type_code<std::list<int>>() != get_type_code<std::list<float>>(),
      "list<T> with different T should get different MD5");
  static_assert(
      get_type_code<std::set<int>>() != get_type_code<std::set<float>>(),
      "set<T> with different T should get different MD5");
  static_assert(get_type_code<std::map<int, std::string>>() !=
                    get_type_code<std::set<float, std::string>>(),
                "map<T1,T2> with different T1 should get different MD5");
  static_assert(get_type_code<std::map<std::string, int>>() !=
                    get_type_code<std::map<std::string, float>>(),
                "map<T1,T2> with different T2 should get different MD5");
  static_assert(
      get_type_code<std::unordered_map<int, std::string>>() !=
          get_type_code<std::unordered_map<float, std::string>>(),
      "unordered_map<T1,T2> with different T1 should get different MD5");
  static_assert(
      get_type_code<std::unordered_map<std::string, int>>() !=
          get_type_code<std::unordered_map<std::string, float>>(),
      "unordered_map<T1,T2> with different T2 should get different MD5");
  static_assert(get_type_code<std::unordered_set<int>>() !=
                    get_type_code<std::unordered_set<float>>(),
                "unordered_set<T> with different T should get different MD5");
  static_assert(get_type_code<std::array<int, 5>>() !=
                    get_type_code<std::array<float, 5>>(),
                "array<T,sz> with different T should get different MD5");
  static_assert(get_type_code<std::array<int, 5>>() !=
                    get_type_code<std::array<int, 6>>(),
                "array<T,sz> with different sz should get different MD5");
  static_assert(get_type_code<int[5]>() != get_type_code<float[5]>(),
                "T[sz] with different T should get different MD5");
  static_assert(get_type_code<int[5]>() != get_type_code<int[6]>(),
                "T[sz] with different sz should get different MD5");
  static_assert(get_type_code<std::optional<int>>() !=
                    get_type_code<std::optional<float>>(),
                "optional<T> with different T should get different MD5");
  static_assert(
      get_type_code<int, float, int>() != get_type_code<int, int, int>(),
      "T... with different T... should get different MD5");
  static_assert(get_type_code<std::tuple<int, float, int>>() !=
                    get_type_code<std::tuple<int, int, int>>(),
                "tuple<T...> with different T... should get different MD5");
  static_assert(get_type_code<std::tuple<int, float, int>>() ==
                    get_type_code<int, float, int>(),
                "tuple<T...> and T... should get same MD5");
  static_assert(get_type_code<std::pair<int, int>>() !=
                    get_type_code<std::pair<float, int>>(),
                "pair<T1,T2> with different T1 should get different MD5");
  static_assert(get_type_code<std::pair<int, float>>() !=
                    get_type_code<std::pair<int, int>>(),
                "pair<T1,T2> with different T2 should get different MD5");
  static_assert(get_type_code<complicated_object>() != get_type_code<person>(),
                "class{T...} with different T... should get different MD5");

  static_assert(
      get_type_code<compatible<int>>() == get_type_code<compatible<float>>(),
      "compatible<T> with different T should get same MD5");
  static_assert(
      get_type_code<std::list<int>>() == get_type_code<std::vector<int>>(),
      "different class accord with container concept should get same MD5");
  static_assert(
      get_type_code<std::deque<int>>() == get_type_code<std::vector<int>>(),
      "different class accord with container concept should get same MD5");
  static_assert(
      get_type_code<std::deque<int>>() == get_type_code<std::list<int>>(),
      "different class accord with container concept should get same MD5");
  static_assert(
      get_type_code<std::array<int, 5>>() == get_type_code<int[5]>(),
      "different class accord with array concept should get same MD5");
  static_assert(get_type_code<std::map<int, int>>() ==
                    get_type_code<std::unordered_map<int, int>>(),
                "different class accord with map concept should get same MD5");
  static_assert(get_type_code<std::set<int>>() ==
                    get_type_code<std::unordered_set<int>>(),
                "different class accord with set concept should get same MD5");
  static_assert(
      get_type_code<std::pair<int, std::string>>() == get_type_code<person>(),
      "different class accord with aggregate_class concept should "
      "get same MD5");
  static_assert(
      get_type_code<std::tuple<int, std::string>>() == get_type_code<person>(),
      "different class accord with aggregate_class concept should "
      "get same MD5");
  static_assert(get_type_code<std::pair<int, std::string>>() ==
                    get_type_code<std::tuple<int, std::string>>(),
                "different class accord with aggregate_class concept should "
                "get same MD5");

  static_assert(
      get_type_code<type_calculate_test_1>() ==
          get_type_code<type_calculate_test_2>(),
      "struct type_calculate_test_1 && type_calculate_test_2 should get the "
      "same MD5");

  static_assert(
      get_type_code<type_calculate_test_1>() !=
          get_type_code<type_calculate_test_3>(),
      "struct type_calculate_test_1 && type_calculate_test_3 should get the "
      "different MD5");

  static_assert(get_type_code<std::tuple<int>>() !=
                    get_type_code<std::tuple<std::tuple<int>>>(),
                "T & tuple<T> should get different MD5");

  static_assert(get_type_code<std::tuple<std::tuple<int>, int>>() !=
                    get_type_code<std::tuple<std::tuple<int, int>>>(),
                "{tuple<T1>,T2} & tuple<T1,T2> should get different MD5");
}
