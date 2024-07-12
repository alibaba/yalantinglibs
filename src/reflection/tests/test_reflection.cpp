#include <sstream>
#include <utility>

#include "ylt/reflection/member_names.hpp"
#include "ylt/reflection/member_value.hpp"

#define DOCTEST_CONFIG_IMPLEMENT
#include "doctest.h"

#if __has_include(<concepts>) || defined(__clang__) || defined(_MSC_VER) || \
    (defined(__GNUC__) && __GNUC__ > 10)

using namespace ylt::reflection;

struct sub {
  int id;
};

enum class Color { red, black };

struct person {
  Color color;
  int id;
  sub s;
  std::string str;
  int arr[2];
};

TEST_CASE("test member names") {
  constexpr size_t size = members_count_v<person>;
  CHECK(size == 5);
  constexpr auto tp = struct_to_tuple<person>();
  constexpr size_t tp_size = std::tuple_size_v<decltype(tp)>;
  CHECK(tp_size == 5);

  constexpr auto arr = get_member_names<person>();
  for (auto name : arr) {
    std::cout << name << ", ";
  }
  std::cout << "\n";
  CHECK(arr ==
        std::array<std::string_view, size>{"color", "id", "s", "str", "arr"});
}

struct simple {
  int color;
  int id;
  std::string str;
  int age;
};

void test_pointer() {
  simple t{};
  auto&& [a, b, c, d] = t;
  size_t offset1 = (const char*)(&a) - (const char*)(&t);
  size_t offset2 = (const char*)(&b) - (const char*)(&t);
  size_t offset3 = (const char*)(&c) - (const char*)(&t);
  size_t offset4 = (const char*)(&d) - (const char*)(&t);

  const auto& offset_arr = get_member_offset_arr<simple>();
  CHECK(offset_arr[0] == offset1);
  CHECK(offset_arr[1] == offset2);
  CHECK(offset_arr[2] == offset3);
  CHECK(offset_arr[3] == offset4);
}

TEST_CASE("test member pointer and offset") { test_pointer(); }

TEST_CASE("test member value") {
  simple p{.color = 2, .id = 10, .str = "hello reflection", .age = 6};
  auto ref_tp = object_to_tuple(p);
  constexpr auto arr = get_member_names<simple>();
  std::stringstream out;
  [&]<size_t... Is>(std::index_sequence<Is...>) {
    ((out << "name: " << arr[Is] << ", value: " << std::get<Is>(ref_tp)
          << "\n"),
     ...);
  }
  (std::make_index_sequence<arr.size()>{});

  std::string result = out.str();
  std::cout << out.str();

  std::string expected_str =
      "name: color, value: 2\nname: id, value: 10\nname: str, value: hello "
      "reflection\nname: age, value: 6\n";
  CHECK(result == expected_str);

  constexpr auto map = get_member_names_map<simple>();
  constexpr size_t index = map.at("age");
  CHECK(index == 3);
  auto age = std::get<index>(ref_tp);
  CHECK(age == 6);

  auto& age1 = get<int>(p, "age");
  CHECK(age1 == 6);

  auto& age2 = get<"age"_ylts>(p);
  CHECK(age2 == 6);

  auto& var1 = get<"str"_ylts>(p);
  CHECK(var1 == "hello reflection");

  auto age3 = get<int>(p, 3);
  CHECK(age3 == 6);

  auto str2 = get<2>(p);
  CHECK(str2 == "hello reflection");

  auto var = get(p, 3);
  CHECK(*std::get<3>(var) == 6);

  auto var2 = get(p, "str");
  CHECK(*std::get<2>(var2) == "hello reflection");
  std::visit(
      [](auto ptr) {
        std::cout << *ptr << "\n";
      },
      var2);

  for_each(p, [](auto& field) {
    std::cout << field << "\n";
  });

  for_each(p, [](auto& field, auto name, auto index) {
    std::cout << field << ", " << name << ", " << index << "\n";
  });

  for_each(p, [](auto& field, auto name) {
    std::cout << field << ", " << name << "\n";
  });

  for_each<simple>([](std::string_view field_name, size_t index) {
    std::cout << index << ", " << field_name << "\n";
  });

  for_each<simple>([](std::string_view field_name) {
    std::cout << field_name << "\n";
  });

  constexpr std::string_view name1 = name_of<simple, 2>();
  CHECK(name1 == "str");

  constexpr std::string_view name2 = name_of<simple>(2);
  CHECK(name2 == "str");

  constexpr size_t idx = index_of<simple, "str"_ylts>();
  CHECK(idx == 2);

  constexpr size_t idx1 = index_of<simple>("str");
  CHECK(idx1 == 2);

  constexpr size_t idx2 = index_of<simple, "no_such"_ylts>();
  CHECK(idx2 == 4);

  size_t idx3 = index_of<simple>("no_such");
  CHECK(idx3 == 4);

  size_t idx4 = index_of(p, age1);
  CHECK(idx4 == 3);

  auto name3 = name_of(p, age1);
  CHECK(name3 == "age");

  int no_such = 100;
  size_t idx5 = index_of(p, no_such);
  CHECK(idx5 == 4);
}

#endif

DOCTEST_MSVC_SUPPRESS_WARNING_WITH_PUSH(4007)
int main(int argc, char** argv) { return doctest::Context(argc, argv).run(); }
DOCTEST_MSVC_SUPPRESS_WARNING_POP
