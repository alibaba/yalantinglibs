#include <memory>

#include "doctest.h"
#include "struct_pack/struct_pack.hpp"
#include "test_struct.hpp"

using namespace struct_pack;

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

struct type_calculate_test_1 {
  tuplet::tuple<
      std::pair<std::map<int, std::string>, std::vector<std::list<int>>>,
      std::set<int>, std::array<int64_t, 64>, std::optional<int>>
      a;
};
struct type_calculate_test_2 {
  struct {
    tuplet::tuple<std::unordered_map<int, std::string>,
                  std::deque<std::vector<int>>>
        a;
    std::multiset<int> b;
    int64_t c[64];
    std::optional<int> d;
  } e;
};

#ifndef NDEBUG
TEST_CASE("test hash conflict detected") {
  int32_t value = 42;
  auto ret = serialize(value);
  auto fake_hash = struct_pack::get_type_code<float>() | 0b1;
  memcpy(ret.data(), &fake_hash, sizeof(fake_hash));
  auto res = deserialize<float>(ret);
  CHECK(!res);
  CHECK(res.error() == struct_pack::errc::hash_conflict);
}
#endif

struct type_calculate_test_3 {
  struct {
    std::pair<std::unordered_map<int, std::string>,
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

  {
    static_assert(get_type_code<std::vector<int>>() !=
                      get_type_code<std::vector<float>>(),
                  "vector<T> with different T should get different MD5");
    CHECK(!deserialize<std::vector<int>>(serialize(std::vector<float>{})));
  }
  {
    static_assert(
        get_type_code<std::deque<int>>() != get_type_code<std::deque<float>>(),
        "deque<T> with different T should get different MD5");
    CHECK(!deserialize<std::deque<int>>(serialize(std::deque<float>{})));
  }
  {
    static_assert(
        get_type_code<std::list<int>>() != get_type_code<std::list<float>>(),
        "list<T> with different T should get different MD5");
    CHECK(!deserialize<std::list<int>>(serialize(std::list<float>{})));
  }
  {
    static_assert(
        get_type_code<std::set<int>>() != get_type_code<std::set<float>>(),
        "set<T> with different T should get different MD5");
    CHECK(!deserialize<std::set<int>>(serialize(std::set<float>{})));
  }
  {
    static_assert(get_type_code<std::map<int, std::string>>() !=
                      get_type_code<std::map<float, std::string>>(),
                  "map<T1,T2> with different T1 should get different MD5");
    CHECK(!deserialize<std::map<int, std::string>>(
        serialize(std::map<float, std::string>{})));
  }

  {
    static_assert(get_type_code<std::map<std::string, int>>() !=
                      get_type_code<std::map<std::string, float>>(),
                  "map<T1,T2> with different T2 should get different MD5");
    CHECK(!deserialize<std::map<std::string, int>>(
        serialize(std::map<std::string, float>{})));
  }
  {
    static_assert(
        get_type_code<std::unordered_map<int, std::string>>() !=
            get_type_code<std::unordered_map<float, std::string>>(),
        "unordered_map<T1,T2> with different T1 should get different MD5");
    CHECK(!deserialize<std::unordered_map<int, std::string>>(
        serialize(std::unordered_map<float, std::string>{})));
  }
  {
    static_assert(
        get_type_code<std::unordered_map<std::string, int>>() !=
            get_type_code<std::unordered_map<std::string, float>>(),
        "unordered_map<T1,T2> with different T2 should get different MD5");
    CHECK(!deserialize<std::unordered_map<std::string, int>>(
        serialize(std::unordered_map<std::string, float>{})));
  }
  {
    static_assert(get_type_code<std::unordered_set<int>>() !=
                      get_type_code<std::unordered_set<float>>(),
                  "unordered_set<T> with different T should get different MD5");
    CHECK(!deserialize<std::unordered_set<int>>(
        serialize(std::unordered_set<float>{})));
  }
  {
    static_assert(get_type_code<std::array<int, 5>>() !=
                      get_type_code<std::array<float, 5>>(),
                  "array<T,sz> with different T should get different MD5");
    CHECK(!deserialize<std::array<int, 5>>(serialize(std::array<float, 5>{})));
  }
  {
    static_assert(get_type_code<std::array<int, 5>>() !=
                      get_type_code<std::array<int, 6>>(),
                  "array<T,sz> with different sz should get different MD5");
    CHECK(!deserialize<std::array<int, 5>>(serialize(std::array<int, 6>{})));
  }
  {
    static_assert(get_type_code<int[5]>() != get_type_code<float[5]>(),
                  "T[sz] with different T should get different MD5");
    int ar[5] = {};
    float ar2[5] = {};
    CHECK(deserialize_to(ar2, serialize(ar)) != struct_pack::errc{});
  }
  {
    static_assert(get_type_code<int[5]>() != get_type_code<int[6]>(),
                  "T[sz] with different sz should get different MD5");
    int ar[5] = {};
    int ar2[6] = {};
    CHECK(deserialize_to(ar2, serialize(ar)) != struct_pack::errc{});
  }
  {
    static_assert(get_type_code<std::optional<int>>() !=
                      get_type_code<std::optional<float>>(),
                  "optional<T> with different T should get different MD5");
    CHECK(!deserialize<std::array<int, 5>>(serialize(std::array<int, 6>{})));
  }
  {
    static_assert(
        get_type_code<int, float, int>() != get_type_code<int, int, int>(),
        "T... with different T... should get different MD5");
    CHECK(!deserialize<int, int>(serialize(1, 2, 3)));
  }
  {
    static_assert(get_type_code<std::tuple<int, float, int>>() !=
                      get_type_code<std::tuple<int, int, int>>(),
                  "tuple<T...> with different T... should get different MD5");
    CHECK(!deserialize<int, int>(serialize(1, 2, 3)));
  }
  {
    static_assert(get_type_code<std::tuple<int, float, int>>() ==
                      get_type_code<int, float, int>(),
                  "tuple<T...> and T... should get same MD5");
    CHECK(deserialize<std::tuple<int, float, int>>(serialize(1, 2.0f, 3)));
  }
  {
    static_assert(get_type_code<std::pair<int, int>>() !=
                      get_type_code<std::pair<float, int>>(),
                  "pair<T1,T2> with different T1 should get different MD5");
    CHECK(!deserialize<std::pair<int, int>>(serialize(std::pair{1.3f, 1})));
  }
  {
    static_assert(get_type_code<std::pair<int, float>>() !=
                      get_type_code<std::pair<int, int>>(),
                  "pair<T1,T2> with different T2 should get different MD5");
    CHECK(!deserialize<std::pair<int, float>>(serialize(std::pair{1, 1})));
  }
  {
    static_assert(
        get_type_code<complicated_object>() != get_type_code<person>(),
        "class{T...} with different T... should get different MD5");
    CHECK(!deserialize<complicated_object>(serialize(person{})));
  }
  {
    static_assert(
        get_type_code<compatible<int>>() == get_type_code<compatible<float>>(),
        "compatible<T> with different T should get same MD5");
    CHECK(deserialize<compatible<int>>(serialize(compatible<float>{1})));
  }
  {
    static_assert(
        get_type_code<std::list<int>>() == get_type_code<std::vector<int>>(),
        "different class accord with container concept should get same MD5");
    CHECK(deserialize<std::list<int>>(serialize(std::vector<int>{})));
  }
  {
    static_assert(
        get_type_code<std::deque<int>>() == get_type_code<std::vector<int>>(),
        "different class accord with container concept should get same MD5");
    CHECK(deserialize<std::deque<int>>(serialize(std::vector<int>{})));
  }
  {
    static_assert(
        get_type_code<std::deque<int>>() == get_type_code<std::list<int>>(),
        "different class accord with container concept should get same MD5");
    CHECK(deserialize<std::deque<int>>(serialize(std::list<int>{})));
  }
  {
    static_assert(
        get_type_code<std::array<int, 5>>() == get_type_code<int[5]>(),
        "different class accord with array concept should get same MD5");
    int ar[5] = {};
    CHECK(deserialize<std::array<int, 5>>(serialize(ar)));
  }
  {
    static_assert(
        get_type_code<std::map<int, int>>() ==
            get_type_code<std::unordered_map<int, int>>(),
        "different class accord with map concept should get same MD5");
    CHECK(deserialize<std::map<int, int>>(
        serialize(std::unordered_map<int, int>{})));
  }
  {
    static_assert(
        get_type_code<std::set<int>>() ==
            get_type_code<std::unordered_set<int>>(),
        "different class accord with set concept should get same MD5");
    CHECK(deserialize<std::set<int>>(serialize(std::unordered_set<int>{})));
  }
  {
    static_assert(
        get_type_code<std::pair<int, std::string>>() == get_type_code<person>(),
        "different class accord with trival_class concept should "
        "get same MD5");
    CHECK(deserialize<std::pair<int, std::string>>(serialize(person{})));
  }
  {
    static_assert(get_type_code<tuplet::tuple<int, std::string>>() ==
                      get_type_code<person>(),
                  "different class accord with trival_class concept should "
                  "get same MD5");
    CHECK(deserialize<tuplet::tuple<int, std::string>>(serialize(person{})));
  }
  {
    static_assert(get_type_code<std::pair<int, std::string>>() ==
                      get_type_code<tuplet::tuple<int, std::string>>(),
                  "different class accord with trival_class concept should "
                  "get same MD5");
    CHECK(deserialize<std::pair<int, std::string>>(
        serialize(tuplet::tuple<int, std::string>{})));
  }
  {
    static_assert(get_type_code<std::tuple<int, std::string>>() !=
                      get_type_code<person>(),
                  "different class accord and trival_class"
                  "concept should get different MD5");
    CHECK(!deserialize<std::tuple<int, std::string>>(serialize(person{})));
  }
  {
    static_assert(get_type_code<std::pair<int, std::string>>() !=
                      get_type_code<std::tuple<int, std::string>>(),
                  "different class accord and trival_class concept should "
                  "get different MD5");
    CHECK(!deserialize<std::pair<int, std::string>>(
        serialize(std::tuple<int, std::string>{})));
  }
  {
    static_assert(get_type_code<type_calculate_test_1>() ==
                      get_type_code<type_calculate_test_2>(),
                  "struct type_calculate_test_1 && type_calculate_test_2 "
                  "should get the "
                  "same MD5");
    CHECK(
        deserialize<type_calculate_test_1>(serialize(type_calculate_test_2{})));
  }

  {
    static_assert(get_type_code<type_calculate_test_1>() !=
                      get_type_code<type_calculate_test_3>(),
                  "struct type_calculate_test_1 && type_calculate_test_3 "
                  "should get the "
                  "different MD5");
    CHECK(!deserialize<type_calculate_test_1>(
        serialize(type_calculate_test_3{})));
  }

  {
    static_assert(get_type_code<int>() != get_type_code<std::tuple<int>>(),
                  "T & tuple<T> should get different MD5");
    CHECK(!deserialize<int>(serialize(std::tuple<int>{})));
  }

  {
    static_assert(get_type_code<int>() != get_type_code<tuplet::tuple<int>>(),
                  "T & tuple_::tuple<T> should get different MD5");
    CHECK(!deserialize<int>(serialize(tuplet::tuple<int>{})));
  }

  {
    static_assert(get_type_code<std::tuple<int>>() !=
                      get_type_code<std::tuple<std::tuple<int>>>(),
                  "tuple<T> & tuple<tuple<T>> should get different MD5");
    CHECK(!deserialize<std::tuple<int>>(
        serialize(std::tuple<std::tuple<int>>{})));
  }

  {
    static_assert(
        get_type_code<std::tuple<int>, int>() !=
            get_type_code<std::tuple<int, int>>(),
        "tuple<tuple<T1>,T2> & tuple<tuple<T1,T2>> should get different MD5");
    CHECK(!deserialize<std::tuple<std::tuple<int>, int>>(
        serialize(std::tuple<std::tuple<int, int>>{})));
  }
}
