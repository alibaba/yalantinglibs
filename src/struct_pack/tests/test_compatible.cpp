#include "doctest.h"
#include "struct_pack/struct_pack.hpp"
#include "struct_pack/struct_pack/struct_pack_impl.hpp"
#include "test_struct.hpp"

using namespace struct_pack;

struct compatible12;
struct compatible1 {
  struct_pack::compatible<int> c;
  friend bool operator<(const compatible1& self, const compatible1& other) {
    if (self.c.has_value()) {
      if (other.c.has_value()) {
        return self.c.value() < other.c.value();
      }
      else {
        return false;
      }
    }
    else if (other.c.has_value()) {
      return true;
    }
    else
      return false;
  }
};

struct compatible2 {
  std::string a;
  compatible1 comp;
  std::string b;
};

struct compatible3 {
  std::string a;
  std::tuple<int, double, compatible2, char> comp;
  std::string b;
};

struct compatible4 {
  std::string a;
  std::optional<compatible3> comp;
  std::string b;
  friend bool operator<(const compatible4&, const compatible4&) {
    return false;
  }
};

struct compatible10 {
  std::string a;
  std::vector<compatible4> j;
  std::string b;
};

struct compatible11 {
  std::string a;
  std::variant<int, compatible10> j;
  std::string b;
};

struct compatible12 {
  std::string a;
  std::unique_ptr<compatible10> j;
  std::string b;
};

TEST_CASE("test compatible check") {
  static_assert(!struct_pack::detail::exist_compatible_member<nested_object>,
                "check compatible failed");
  static_assert(struct_pack::detail::exist_compatible_member<compatible1>,
                "check compatible failed");
  static_assert(struct_pack::detail::exist_compatible_member<compatible2>,
                "check compatible failed");
  static_assert(struct_pack::detail::exist_compatible_member<compatible3>,
                "check compatible failed");
  static_assert(struct_pack::detail::exist_compatible_member<compatible4>,
                "check compatible failed");
  static_assert(struct_pack::detail::exist_compatible_member<compatible10>,
                "check compatible failed");
  static_assert(struct_pack::detail::exist_compatible_member<compatible11>,
                "check compatible failed");
  static_assert(struct_pack::detail::exist_compatible_member<compatible12>,
                "check compatible failed");
};

TEST_CASE("test compatible") {
  SUBCASE("serialize person1 2 person") {
    person1 p1{20, "tom", 1, false};
    std::vector<char> buffer;
    auto size = get_needed_size(p1);
    buffer.resize(size);
    serialize_to(buffer.data(), size, p1);

    person p;
    auto res = deserialize_to(p, buffer.data(), buffer.size());
    CHECK(res == struct_pack::errc{});
    CHECK(p.name == p1.name);
    CHECK(p.age == p1.age);

    auto size2 = get_needed_size(p);
    // short data
    for (int i = 0, lim = size2; i < lim; ++i)
      CHECK(deserialize_to(p, buffer.data(), i) ==
            struct_pack::errc::no_buffer_space);

    serialize_to(buffer.data(), size2, p);

    person1 p2;
    CHECK(deserialize_to(p2, buffer.data(), buffer.size()) ==
          struct_pack::errc{});
    CHECK((p2.age == p.age && p2.name == p.name));
  }
  SUBCASE("serialize person 2 person1") {
    std::vector<char> buffer;
    person p{20, "tom"};
    struct_pack::serialize_to(buffer, p);
    struct_pack::serialize_to(buffer, p);
    struct_pack::serialize_to(buffer, p);

    person1 p1, p0 = {.age = 20, .name = "tom"};
    auto ec = struct_pack::deserialize_to(p1, buffer);
    CHECK(ec == struct_pack::errc{});
    CHECK(p1 == p0);
  }
  SUBCASE("big compatible metainfo") {
    {
#ifdef NDEBUG
      constexpr size_t array_sz = 247;
#else
      constexpr size_t array_sz = 244;
#endif
      std::tuple<compatible<std::array<char, array_sz>>> big =
          std::array<char, array_sz>{'A', 'E', 'I', 'O', 'U'};
      auto sz = get_needed_size(big);
      CHECK(sz == 255);
      auto buffer = serialize(big);
      CHECK(sz == buffer.size());
      CHECK(buffer[0] % 2 == 1);
      CHECK((buffer[4] & 0b11) == 1);
      std::size_t r_sz = 0;
      memcpy(&r_sz, &buffer[5], 2);
      CHECK(r_sz == sz);
      auto big2 =
          deserialize<std::tuple<compatible<std::array<char, array_sz>>>>(
              buffer);
      CHECK(big2);
      CHECK(std::get<0>(big2.value()).value() == std::get<0>(big).value());
    }
    {
#ifdef NDEBUG
      constexpr size_t array_sz = 248;
#else
      constexpr size_t array_sz = 245;
#endif
      std::tuple<compatible<std::array<char, array_sz>>> big =
          std::array<char, array_sz>{'A', 'E', 'I', 'O', 'U'};
      auto sz = get_needed_size(big);
      CHECK(sz == 256);
      auto buffer = serialize(big);
      CHECK(sz == buffer.size());
      CHECK(buffer[0] % 2 == 1);
      CHECK((buffer[4] & 0b11) == 1);
      std::size_t r_sz = 0;
      memcpy(&r_sz, &buffer[5], 2);
      CHECK(r_sz == sz);
      auto big2 =
          deserialize<std::tuple<compatible<std::array<char, array_sz>>>>(
              buffer);
      CHECK(big2);
      CHECK(std::get<0>(big2.value()).value() == std::get<0>(big).value());
    }
    {
#ifdef NDEBUG
      constexpr size_t array_sz = 65525;
#else
      constexpr size_t array_sz = 65522;
#endif
      std::tuple<compatible<std::string>> big = {std::string{"Hello"}};
      std::get<0>(big).value().resize(array_sz);
      auto sz = get_needed_size(big);
      CHECK(sz == 65535);
      auto buffer = serialize(big);
      CHECK(sz == buffer.size());
      CHECK(buffer[0] % 2 == 1);
      CHECK((buffer[4] & 0b11) == 1);
      std::size_t r_sz = 0;
      memcpy(&r_sz, &buffer[5], 2);
      CHECK(r_sz == sz);
      auto big2 = deserialize<std::tuple<compatible<std::string>>>(buffer);
      CHECK(big2);
      CHECK(std::get<0>(big2.value()).value() == std::get<0>(big).value());
    }
    {
#ifdef NDEBUG
      constexpr size_t array_sz = 65526;
#else
      constexpr size_t array_sz = 65523;
#endif
      std::tuple<compatible<std::string>> big = {std::string{"Hello"}};
      std::get<0>(big).value().resize(array_sz);
      auto sz = get_needed_size(big);
      CHECK(sz == 65538);
      auto buffer = serialize(big);
      CHECK(sz == buffer.size());
      CHECK(buffer[0] % 2 == 1);
      CHECK((buffer[4] & 0b11) == 2);
      std::size_t r_sz = 0;
      memcpy(&r_sz, &buffer[5], 4);
      CHECK(r_sz == sz);
      auto big2 = deserialize<std::tuple<compatible<std::string>>>(buffer);
      CHECK(big2);
      CHECK(std::get<0>(big2.value()).value() == std::get<0>(big).value());
    }
    // TODO: test 8byte-len compatible object
  }
}

struct compatible_nested1_old {
  std::string a;
  struct nested {
    std::string a;
    std::string c;
    bool operator==(const nested&) const = default;
  } b;
  std::string c;
  bool operator==(const compatible_nested1_old&) const = default;
};

struct compatible_nested1 {
  std::string a;
  struct nested {
    std::string a;
    struct_pack::compatible<std::string> b;
    std::string c;
    bool operator==(const nested&) const = default;
  } b;
  std::string c;
  bool operator==(const compatible_nested1&) const = default;
};

TEST_CASE("test nested compatible1") {
  compatible_nested1_old obj_old = {"Hello", {"Hi", "Hey"}, "HooooooooooHo"};
  compatible_nested1 obj = {"Hello", {"Hi", "42", "Hey"}, "HooooooooooHo"};
  compatible_nested1 obj_empty = {
      "Hello", {"Hi", std::nullopt, "Hey"}, "HooooooooooHo"};
  SUBCASE("test compatible_nested1 to compatible_nested1") {
    auto buffer = struct_pack::serialize(obj);
    auto result = struct_pack::deserialize<compatible_nested1>(buffer);
    CHECK(result == obj);
  }
  SUBCASE("test compatible_nested1_old to compatible_nested1") {
    auto buffer = struct_pack::serialize(obj_old);
    auto result = struct_pack::deserialize<compatible_nested1>(buffer);
    CHECK(result == obj_empty);
  }
  SUBCASE("test compatible_nested1 to compatible_nested1_old") {
    auto buffer = struct_pack::serialize(obj);
    auto result = struct_pack::deserialize<compatible_nested1_old>(buffer);
    CHECK(result == obj_old);
  }
}

struct compatible_nested2_old {
  std::string a;
  struct nested {
    std::string a;
    std::string c;
    bool operator==(const nested&) const = default;
  };
  std::vector<nested> b;
  std::string c;
  bool operator==(const compatible_nested2_old&) const = default;
};

struct compatible_nested2 {
  std::string a;
  struct nested {
    std::string a;
    struct_pack::compatible<std::string> b;
    std::string c;
    bool operator==(const nested&) const = default;
  };
  std::vector<nested> b;
  std::string c;
  bool operator==(const compatible_nested2&) const = default;
};

TEST_CASE("test nested compatible2") {
  compatible_nested2_old obj_old = {
      "Hello", {{"Hi1", "Hey1"}, {"Hi2", "Hey2"}}, "HooooooooooHo"};
  compatible_nested2 obj = {
      "Hello", {{"Hi1", "42", "Hey1"}, {"Hi2", "43", "Hey2"}}, "HooooooooooHo"};
  compatible_nested2 obj_empty = {
      "Hello",
      {{"Hi1", std::nullopt, "Hey1"}, {"Hi2", std::nullopt, "Hey2"}},
      "HooooooooooHo"};
  SUBCASE("test compatible_nested2 to compatible_nested2") {
    auto buffer = struct_pack::serialize(obj);
    auto result = struct_pack::deserialize<compatible_nested2>(buffer);
    CHECK(result == obj);
  }
  SUBCASE("test compatible_nested2_old to compatible_nested2") {
    auto buffer = struct_pack::serialize(obj_old);
    auto result = struct_pack::deserialize<compatible_nested2>(buffer);
    CHECK(result == obj_empty);
  }
  SUBCASE("test compatible_nested2 to compatible_nested2_old") {
    auto buffer = struct_pack::serialize(obj);
    auto result = struct_pack::deserialize<compatible_nested2_old>(buffer);
    CHECK(result == obj_old);
  }
}

struct compatible_nested3_old {
  std::string a;
  struct nested {
    std::string a;
    std::string c;
    bool operator==(const nested&) const = default;
  };
  std::optional<nested> b;
  std::string c;
  bool operator==(const compatible_nested3_old&) const = default;
};
struct compatible_nested3 {
  std::string a;
  struct nested {
    std::string a;
    struct_pack::compatible<std::string> b;
    std::string c;
    bool operator==(const nested&) const = default;
  };
  std::optional<nested> b;
  std::string c;
  bool operator==(const compatible_nested3&) const = default;
};

TEST_CASE("test nested compatible3") {
  compatible_nested3_old obj_old = {"Hello", {{"Hi", "Hey"}}, "HooooooooooHo"};
  compatible_nested3 obj = {"Hello", {{"Hi", "42", "Hey"}}, "HooooooooooHo"};
  compatible_nested3 obj_empty = {
      "Hello", {{"Hi", std::nullopt, "Hey"}}, "HooooooooooHo"};
  SUBCASE("test compatible_nested3 to compatible_nested3") {
    auto buffer = struct_pack::serialize(obj);
    auto result = struct_pack::deserialize<compatible_nested3>(buffer);
    CHECK(result == obj);
  }
  SUBCASE("test compatible_nested3_old to compatible_nested3") {
    auto buffer = struct_pack::serialize(obj_old);
    auto result = struct_pack::deserialize<compatible_nested3>(buffer);
    CHECK(result == obj_empty);
  }
  SUBCASE("test compatible_nested3 to compatible_nested3_old") {
    auto buffer = struct_pack::serialize(obj);
    auto result = struct_pack::deserialize<compatible_nested3_old>(buffer);
    CHECK(result == obj_old);
  }
}

struct compatible_nested4_old {
  std::string a;
  struct nested {
    std::string a;
    std::string c;
    bool operator==(const nested&) const = default;
  };
  struct_pack::expected<nested, int> b;
  std::string c;
  bool operator==(const compatible_nested4_old&) const = default;
};
struct compatible_nested4 {
  std::string a;
  struct nested {
    std::string a;
    struct_pack::compatible<std::string> b;
    std::string c;
    bool operator==(const nested&) const = default;
  };
  struct_pack::expected<nested, int> b;
  std::string c;
  bool operator==(const compatible_nested4&) const = default;
};

TEST_CASE("test nested compatible4") {
  compatible_nested4_old obj_old = {"Hello", {{"Hi", "Hey"}}, "HooooooooooHo"};
  compatible_nested4 obj = {"Hello", {{"Hi", "42", "Hey"}}, "HooooooooooHo"};
  compatible_nested4 obj_empty = {
      "Hello", {{"Hi", std::nullopt, "Hey"}}, "HooooooooooHo"};
  SUBCASE("test compatible_nested4 to compatible_nested4") {
    auto buffer = struct_pack::serialize(obj);
    auto result = struct_pack::deserialize<compatible_nested4>(buffer);
    CHECK(result == obj);
  }
  SUBCASE("test compatible_nested4_old to compatible_nested4") {
    auto buffer = struct_pack::serialize(obj_old);
    auto result = struct_pack::deserialize<compatible_nested4>(buffer);
    CHECK(result == obj_empty);
  }
  SUBCASE("test compatible_nested4 to compatible_nested4_old") {
    auto buffer = struct_pack::serialize(obj);
    auto result = struct_pack::deserialize<compatible_nested4_old>(buffer);
    CHECK(result == obj_old);
  }
}

struct compatible_nested5_old {
  std::string a;
  struct nested {
    std::string a;
    std::string c;
    bool operator==(const nested&) const = default;
  };
  struct_pack::expected<int, nested> b;
  std::string c;
  bool operator==(const compatible_nested5_old&) const = default;
};
struct compatible_nested5 {
  std::string a;
  struct nested {
    std::string a;
    struct_pack::compatible<std::string> b;
    std::string c;
    bool operator==(const nested&) const = default;
  };
  struct_pack::expected<int, nested> b;
  std::string c;
  bool operator==(const compatible_nested5&) const = default;
};

TEST_CASE("test nested compatible5") {
  compatible_nested5_old obj_old = {
      "Hello",
      struct_pack::unexpected<compatible_nested5_old::nested>{{"Hi", "Hey"}},
      "HooooooooooHo"};
  compatible_nested5 obj = {
      "Hello",
      struct_pack::unexpected<compatible_nested5::nested>{{"Hi", "42", "Hey"}},
      "HooooooooooHo"};
  compatible_nested5 obj_empty = {
      "Hello",
      struct_pack::unexpected<compatible_nested5::nested>{
          {"Hi", std::nullopt, "Hey"}},
      "HooooooooooHo"};
  SUBCASE("test compatible_nested5 to compatible_nested5") {
    auto buffer = struct_pack::serialize(obj);
    auto result = struct_pack::deserialize<compatible_nested5>(buffer);
    CHECK(result == obj);
  }
  SUBCASE("test compatible_nested5_old to compatible_nested5") {
    auto buffer = struct_pack::serialize(obj_old);
    auto result = struct_pack::deserialize<compatible_nested5>(buffer);
    CHECK(result == obj_empty);
  }
  SUBCASE("test compatible_nested5 to compatible_nested5_old") {
    auto buffer = struct_pack::serialize(obj);
    auto result = struct_pack::deserialize<compatible_nested5_old>(buffer);
    CHECK(result == obj_old);
  }
}

struct compatible_nested6_old {
  std::string a;
  struct nested {
    std::string a;
    std::string c;
    bool operator==(const nested&) const = default;
  };
  std::variant<int, nested> b;
  std::string c;
  bool operator==(const compatible_nested6_old&) const = default;
};
struct compatible_nested6 {
  std::string a;
  struct nested {
    std::string a;
    struct_pack::compatible<std::string> b;
    std::string c;
    bool operator==(const nested&) const = default;
  };
  std::variant<int, nested> b;
  std::string c;
  bool operator==(const compatible_nested6&) const = default;
};

TEST_CASE("test nested compatible6") {
  compatible_nested6_old obj_old = {
      "Hello", compatible_nested6_old::nested{"Hi", "Hey"}, "HooooooooooHo"};
  compatible_nested6 obj = {
      "Hello", compatible_nested6::nested{"Hi", "42", "Hey"}, "HooooooooooHo"};
  compatible_nested6 obj_empty = {
      "Hello", compatible_nested6::nested{"Hi", std::nullopt, "Hey"},
      "HooooooooooHo"};
  SUBCASE("test compatible_nested6 to compatible_nested6") {
    auto buffer = struct_pack::serialize(obj);
    auto result = struct_pack::deserialize<compatible_nested6>(buffer);
    CHECK(result == obj);
  }
  SUBCASE("test compatible_nested6_old to compatible_nested6") {
    auto buffer = struct_pack::serialize(obj_old);
    auto result = struct_pack::deserialize<compatible_nested6>(buffer);
    CHECK(result == obj_empty);
  }
  SUBCASE("test compatible_nested6 to compatible_nested6_old") {
    auto buffer = struct_pack::serialize(obj);
    auto result = struct_pack::deserialize<compatible_nested6_old>(buffer);
    CHECK(result == obj_old);
  }
}

struct compatible_nested7_old {
  std::string a;
  struct nested {
    std::string a;
    std::string c;
    bool operator==(const nested&) const = default;
  };
  std::unique_ptr<nested> b;
  std::string c;
  bool operator==(const compatible_nested7_old& other) const {
    if (this->a == other.a && this->c == other.c) {
      if (this->b != nullptr && other.b != nullptr) {
        return *(this->b) == *(other.b);
      }
      else if (this->b == nullptr && other.b == nullptr) {
        return true;
      }
      else {
        return false;
      }
    }
    else {
      return false;
    }
  }
};

struct compatible_nested7 {
  std::string a;
  struct nested {
    std::string a;
    struct_pack::compatible<std::string> b;
    std::string c;
    bool operator==(const nested&) const = default;
  };
  std::unique_ptr<nested> b;
  std::string c;
  bool operator==(const compatible_nested7& other) const {
    if (this->a == other.a && this->c == other.c) {
      if (this->b != nullptr && other.b != nullptr) {
        return *(this->b) == *(other.b);
      }
      else if (this->b == nullptr && other.b == nullptr) {
        return true;
      }
      else {
        return false;
      }
    }
    else {
      return false;
    }
  }
};
#include <iostream>
TEST_CASE("test nested compatible7") {
  compatible_nested7_old obj_old = {
      "Hello",
      std::make_unique<compatible_nested7_old::nested>(
          compatible_nested7_old::nested{"Hi", "Hey"}),
      "HooooooooooHo"};
  compatible_nested7 obj = {"Hello",
                            std::make_unique<compatible_nested7::nested>(
                                compatible_nested7::nested{"Hi", "42", "Hey"}),
                            "HooooooooooHo"};
  compatible_nested7 obj_empty = {
      "Hello",
      std::make_unique<compatible_nested7::nested>(
          compatible_nested7::nested{"Hi", std::nullopt, "Hey"}),
      "HooooooooooHo"};
  SUBCASE("test compatible_nested7 to compatible_nested7") {
    auto buffer = struct_pack::serialize(obj);
    auto result = struct_pack::deserialize<compatible_nested7>(buffer);
    CHECK(result == obj);
  }
  SUBCASE("test compatible_nested7_old to compatible_nested7") {
    auto buffer = struct_pack::serialize(obj_old);
    auto result = struct_pack::deserialize<compatible_nested7>(buffer);
    CHECK(result == obj_empty);
  }
  SUBCASE("test compatible_nested7 to compatible_nested7_old") {
    auto buffer = struct_pack::serialize(obj);
    auto result = struct_pack::deserialize<compatible_nested7_old>(buffer);
    CHECK(result == obj_old);
  }
}

struct compatible_nested8_old {
  std::string a;
  std::tuple<std::string, std::string> b;
  std::string c;
  bool operator==(const compatible_nested8_old&) const = default;
};

struct compatible_nested8 {
  std::string a;
  std::tuple<std::string, struct_pack::compatible<std::string>, std::string> b;
  std::string c;
  bool operator==(const compatible_nested8&) const = default;
};

TEST_CASE("test nested compatible8") {
  compatible_nested8_old obj_old = {"Hello", {"Hi", "Hey"}, "HooooooooooHo"};
  compatible_nested8 obj = {"Hello", {"Hi", "42", "Hey"}, "HooooooooooHo"};
  compatible_nested8 obj_empty = {
      "Hello", {"Hi", std::nullopt, "Hey"}, "HooooooooooHo"};
  SUBCASE("test compatible_nested8 to compatible_nested8") {
    auto buffer = struct_pack::serialize(obj);
    auto result = struct_pack::deserialize<compatible_nested8>(buffer);
    CHECK(result == obj);
  }
  SUBCASE("test compatible_nested8_old to compatible_nested8") {
    auto buffer = struct_pack::serialize(obj_old);
    auto result = struct_pack::deserialize<compatible_nested8>(buffer);
    CHECK(result == obj_empty);
  }
  SUBCASE("test compatible_nested8 to compatible_nested8_old") {
    auto buffer = struct_pack::serialize(obj);
    auto result = struct_pack::deserialize<compatible_nested8_old>(buffer);
    CHECK(result == obj_old);
  }
}
// TODO: test set, map_key, map_value

struct compatible_nested9_old {
  std::string a;
  struct nested {
    std::string a;
    std::string c;
    bool operator==(const nested&) const = default;
  };
  std::map<std::string, nested> b;
  std::string c;
  bool operator==(const compatible_nested9_old&) const = default;
};

struct compatible_nested9 {
  std::string a;
  struct nested {
    std::string a;
    struct_pack::compatible<std::string> b;
    std::string c;
    bool operator==(const nested&) const = default;
  };
  std::map<std::string, nested> b;
  std::string c;
  bool operator==(const compatible_nested9&) const = default;
};

TEST_CASE("test nested compatible9") {
  compatible_nested9_old obj_old = {
      "Hello", {{"1", {"Hi", "Ho"}}, {"2", {"Hi2", "Ho2"}}}, "HooooooooooHo"};
  compatible_nested9 obj = {
      "Hello",
      {{"1", {"Hi", "Hey", "Ho"}}, {"2", {"Hi2", "Hey2", "Ho2"}}},
      "HooooooooooHo"};
  compatible_nested9 obj_empty = {
      "Hello",
      {{"1", {"Hi", std::nullopt, "Ho"}}, {"2", {"Hi2", std::nullopt, "Ho2"}}},
      "HooooooooooHo"};
  SUBCASE("test compatible_nested9 to compatible_nested9") {
    auto buffer = struct_pack::serialize(obj);
    auto result = struct_pack::deserialize<compatible_nested9>(buffer);
    CHECK(result == obj);
  }
  SUBCASE("test compatible_nested9_old to compatible_nested9") {
    auto buffer = struct_pack::serialize(obj_old);
    auto result = struct_pack::deserialize<compatible_nested9>(buffer);
    CHECK(result == obj_empty);
  }
  SUBCASE("test compatible_nested9 to compatible_nested9_old") {
    auto buffer = struct_pack::serialize(obj);
    auto result = struct_pack::deserialize<compatible_nested9_old>(buffer);
    CHECK(result == obj_old);
  }
}

struct compatible_with_version_0 {
  std::string a;
  struct nested {
    std::string a;
    std::string c;
    bool operator==(const nested&) const = default;
  };
  std::map<std::string, nested> b;
  std::string c;
  bool operator==(const compatible_with_version_0&) const = default;
};

struct compatible_with_version_1 {
  std::string a;
  struct nested {
    std::string a;
    std::string c;
    bool operator==(const nested&) const = default;
  };
  std::map<std::string, nested> b;
  std::string c;
  struct_pack::compatible<std::string> d;
  struct_pack::compatible<std::string> e;
  bool operator==(const compatible_with_version_1&) const = default;
};

struct compatible_with_version_20230110 {
  std::string a;
  struct nested {
    std::string a;
    struct_pack::compatible<std::string, 20230110> b;
    std::string c;
    struct_pack::compatible<std::string, 20230110> d;
    bool operator==(const nested&) const = default;
  };
  std::map<std::string, nested> b;
  std::string c;
  struct_pack::compatible<std::string> d;
  struct_pack::compatible<std::string> e;
  bool operator==(const compatible_with_version_20230110&) const = default;
};

TEST_CASE("test compatible version number") {
  static_assert(struct_pack::detail::compatible_version_number<
                    compatible_with_version_20230110> ==
                std::array<uint64_t, 2>{0, 20230110});
}

TEST_CASE("test compatible_with_version") {
  compatible_with_version_0 obj_0 = {
      "Hello", {{"1", {"Hi", "Ho"}}, {"2", {"Hi2", "Ho2"}}}, "HooooooooooHo"};
  compatible_with_version_1 obj_1 = {
      "Hello",
      {{"1", {"Hi", "Ho"}}, {"2", {"Hi2", "Ho2"}}},
      "HooooooooooHo",
      "42",
      "43"};
  compatible_with_version_1 obj_1_empty = {
      "Hello",
      {{"1", {"Hi", "Ho"}}, {"2", {"Hi2", "Ho2"}}},
      "HooooooooooHo",
      std::nullopt,
      std::nullopt};
  compatible_with_version_20230110 obj_20230110 = {
      "Hello",
      {{"1", {"Hi", "11", "Ho", "12"}}, {"2", {"Hi2", "21", "Ho2", "22"}}},
      "HooooooooooHo",
      "42",
      "43"};
  compatible_with_version_20230110 obj_20230110_empty_1 = {
      "Hello",
      {{"1", {"Hi", std::nullopt, "Ho", std::nullopt}},
       {"2", {"Hi2", std::nullopt, "Ho2", std::nullopt}}},
      "HooooooooooHo",
      "42",
      "43"};
  compatible_with_version_20230110 obj_20230110_empty_0 = {
      "Hello",
      {{"1", {"Hi", std::nullopt, "Ho", std::nullopt}},
       {"2", {"Hi2", std::nullopt, "Ho2", std::nullopt}}},
      "HooooooooooHo",
      std::nullopt,
      std::nullopt};
  SUBCASE("test compatible_with_version_1 to compatible_with_version_1") {
    auto buffer = struct_pack::serialize(obj_1);
    auto result = struct_pack::deserialize<compatible_with_version_1>(buffer);
    CHECK(result == obj_1);
  }
  SUBCASE(
      "test compatible_with_version_20230110 to "
      "compatible_with_version_20230110") {
    auto buffer = struct_pack::serialize(obj_20230110);
    auto result =
        struct_pack::deserialize<compatible_with_version_20230110>(buffer);
    CHECK(result == obj_20230110);
  }
  SUBCASE("test compatible_with_version_0 to compatible_with_version_1") {
    auto buffer = struct_pack::serialize(obj_0);
    auto result = struct_pack::deserialize<compatible_with_version_1>(buffer);
    CHECK(result == obj_1_empty);
  }
  SUBCASE("test compatible_with_version_1 to compatible_with_version_0") {
    auto buffer = struct_pack::serialize(obj_1);
    auto result = struct_pack::deserialize<compatible_with_version_0>(buffer);
    CHECK(result == obj_0);
  }
  SUBCASE(
      "test compatible_with_version_0 to compatible_with_version_20230110") {
    auto buffer = struct_pack::serialize(obj_0);
    auto result =
        struct_pack::deserialize<compatible_with_version_20230110>(buffer);
    CHECK(result == obj_20230110_empty_0);
  }
  SUBCASE(
      "test compatible_with_version_20230110 to compatible_with_version_0") {
    auto buffer = struct_pack::serialize(obj_20230110);
    auto result = struct_pack::deserialize<compatible_with_version_0>(buffer);
    CHECK(result == obj_0);
  }
  SUBCASE(
      "test compatible_with_version_20230110 to compatible_with_version_1") {
    auto buffer = struct_pack::serialize(obj_20230110);
    auto result = struct_pack::deserialize<compatible_with_version_1>(buffer);
    CHECK(result == obj_1);
  }
  SUBCASE(
      "test compatible_with_version_1 to compatible_with_version_20230110") {
    auto buffer = struct_pack::serialize(obj_1);
    auto result =
        struct_pack::deserialize<compatible_with_version_20230110>(buffer);
    CHECK(result == obj_20230110_empty_1);
  }
}