#include "doctest.h"
#include "struct_pack/struct_pack.hpp"
#include "test_struct.hpp"
using namespace struct_pack;

void test_container(auto &v) {
  auto ret = serialize(v);

  using T = std::remove_cvref_t<decltype(v)>;
  T v1{};
  auto ec = deserialize_to(v1, ret.data(), ret.size());
  CHECK(ec == struct_pack::errc{});
  CHECK(v == v1);

  v.clear();
  v1.clear();
  ret = serialize(v);
  ec = deserialize_to(v1, ret.data(), ret.size());
  CHECK(ec == struct_pack::errc{});
  CHECK(v1.empty());
  CHECK(v == v1);
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
  CHECK(ec == struct_pack::errc{});
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
  [[maybe_unused]] constexpr auto i = get_type_literal<decltype(tp)>();
  static_assert(get_type_literal<decltype(tp)>() !=
                get_type_literal<std::tuple<int, int>>());

  auto buf = serialize(tp);

  std::tuple<int, int> v{};
  auto ec = deserialize_to(v, buf);
  CHECK(ec != struct_pack::errc{});

  decltype(tp) tp1;
  auto ec2 = deserialize_to(tp1, buf);
  CHECK(ec2 == struct_pack::errc{});
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
  CHECK(ec == struct_pack::errc{});
  CHECK(obj.tp == obj1.tp);
}

void test_c_array(auto &v) {
  auto ret = serialize(v);

  using T = std::remove_cvref_t<decltype(v)>;
  T v1{};
  auto ec = deserialize_to(v1, ret.data(), ret.size());
  REQUIRE(ec == struct_pack::errc{});

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
    CHECK(ec == struct_pack::errc{});
    CHECK(sz == ret.size());
    CHECK(e == e2);
  }
  {
    enum_i8 e{enum_i8::hi};
    auto ret = serialize(e);
    auto ec = deserialize<enum_i32>(ret.data(), ret.size());
    CHECK(!ec);
    if (!ec) {
      CHECK(ec.error() == struct_pack::errc::invalid_argument);
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
    CHECK(ec == struct_pack::errc{});
    CHECK(var2 == var);
  }
  {
    std::variant<int, std::monostate> var = std::monostate{}, var2;
    auto ret = struct_pack::serialize(var);
    auto ec = struct_pack::deserialize_to(var2, ret.data(), ret.size());
    CHECK(ec == struct_pack::errc{});
    CHECK(var2 == var);
  }
  {
    std::variant<int, int, int, std::monostate, double, double, double> var{
        std::in_place_index_t<1>{}, 2},
        var2;
    auto ret = struct_pack::serialize(var);
    auto ec = struct_pack::deserialize_to(var2, ret.data(), ret.size());
    CHECK(ec == struct_pack::errc{});
    CHECK(var2 == var);
    CHECK(var2.index() == 1);
  }
  {
    std::variant<std::monostate, std::string> var{"hello"}, var2;
    auto ret = struct_pack::serialize(var);
    auto ec = struct_pack::deserialize_to(var2, ret.data(), ret.size());
    CHECK(ec == struct_pack::errc{});
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
    CHECK(ec == struct_pack::errc{});
    CHECK(big_variant2 == big_variant);
  }
}