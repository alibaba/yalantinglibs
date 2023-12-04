#include <cstdlib>
#include <vector>

#include "doctest.h"
#include "ylt/struct_pack.hpp"
#include "ylt/struct_pack/error_code.hpp"
#include "ylt/struct_pack/type_id.hpp"
namespace my_name_space {

struct array2D {
  unsigned int x;
  unsigned int y;
  float* p;
  array2D(unsigned int x, unsigned int y) : x(x), y(y) {
    p = (float*)calloc(1ull * x * y, sizeof(float));
  }
  array2D(const array2D&) = delete;
  array2D(array2D&& o) : x(o.x), y(o.y), p(o.p) { o.p = nullptr; };
  array2D& operator=(const array2D&) = delete;
  array2D& operator=(array2D&& o) {
    x = o.x;
    y = o.y;
    p = o.p;
    o.p = nullptr;
    return *this;
  }
  float& operator()(std::size_t i, std::size_t j) { return p[i * y + j]; }
  bool operator==(const array2D& o) const {
    return x == o.x && y == o.y &&
           memcmp(p, o.p, 1ull * x * y * sizeof(float)) == 0;
  }
  array2D() : x(0), y(0), p(nullptr) {}
  ~array2D() { free(p); }
};

std::size_t sp_get_needed_size(const array2D& ar) {
  return 2 * struct_pack::get_write_size(ar.x) +
         struct_pack::get_write_size(ar.p, 1ull * ar.x * ar.y);
}
template <typename Writer>
void sp_serialize_to(Writer& writer, const array2D& ar) {
  struct_pack::write(writer, ar.x);
  struct_pack::write(writer, ar.y);
  struct_pack::write(writer, ar.p, 1ull * ar.x * ar.y);
}

template <typename Reader>
struct_pack::errc sp_deserialize_to(Reader& reader, array2D& ar) {
  if (auto ec = struct_pack::read(reader, ar.x); ec != struct_pack::errc{}) {
    return ec;
  }
  if (auto ec = struct_pack::read(reader, ar.y); ec != struct_pack::errc{}) {
    return ec;
  }
  auto length = 1ull * ar.x * ar.y * sizeof(float);
  if constexpr (struct_pack::checkable_reader_t<Reader>) {
    if (!reader.check(length)) {  // some input(such as memory) allow us check
                                  // length before read data.
      return struct_pack::errc::no_buffer_space;
    }
  }
  ar.p = (float*)malloc(length);
  auto ec = struct_pack::read(reader, ar.p, 1ull * ar.x * ar.y);
  if (ec != struct_pack::errc{}) {
    free(ar.p);
  }
  return ec;
}

template <typename Reader>
struct_pack::errc sp_deserialize_to_with_skip(Reader& reader, array2D& ar) {
  if (auto ec = struct_pack::read(reader, ar.x); ec != struct_pack::errc{}) {
    return ec;
  }
  if (auto ec = struct_pack::read(reader, ar.y); ec != struct_pack::errc{}) {
    return ec;
  }
  auto length = 1ull * ar.x * ar.y * sizeof(float);
  if (!reader.ignore(length)) {
    return struct_pack::errc::no_buffer_space;
  }
  return {};
}

}  // namespace my_name_space

TEST_CASE("test user-defined_type") {
  my_name_space::array2D ar(114, 514);
  ar(1, 6) = 3.14;
  ar(87, 111) = 2.71;
  auto buffer = struct_pack::serialize(ar);
  auto result = struct_pack::deserialize<my_name_space::array2D>(buffer);
  CHECK(result.has_value());
  auto& ar2 = result.value();
  CHECK(ar == result.value());
}

TEST_CASE("test user-defined_type nested") {
  std::vector<my_name_space::array2D> ar;
  ar.emplace_back(11, 22);
  ar.emplace_back(114, 514);
  ar[0](1, 6) = 3.14;
  ar[1](87, 111) = 2.71;
  auto buffer = struct_pack::serialize(ar);
  auto result = struct_pack::deserialize<decltype(ar)>(buffer);
  CHECK(result.has_value());
  CHECK(ar == result);
}

TEST_CASE("test user-defined_type get_field") {
  std::pair<my_name_space::array2D, int> o;
  o.first = {114, 514};
  o.first(1, 6) = 3.14;
  o.first(87, 111) = 2.71;
  o.second = 114514;
  auto buffer = struct_pack::serialize(o);
  auto result = struct_pack::get_field<decltype(o), 1>(buffer);
  CHECK(result.has_value());
  auto& i = result.value();
  CHECK(o.second == i);
}

TEST_CASE("test auto type name") {
  {
    auto name = struct_pack::get_type_literal<my_name_space::array2D>();
    std::string_view sv{name.data(), name.size()};
    CHECK(sv == (char)struct_pack::detail::type_id::user_defined_type +
                    std::string{"my_name_space::array2D"} +
                    (char)struct_pack::detail::type_id::type_end_flag);
  }
  {
    auto name =
        struct_pack::get_type_literal<std::vector<my_name_space::array2D>>();
    std::string_view sv{name.data(), name.size()};
    CHECK(sv == std::string{(char)struct_pack::detail::type_id::container_t} +
                    (char)struct_pack::detail::type_id::user_defined_type +
                    std::string{"my_name_space::array2D"} +
                    (char)struct_pack::detail::type_id::type_end_flag);
  }
}

namespace my_name_space {

struct test {
  unsigned int x;
};

std::size_t sp_get_needed_size(const test& t) {
  return struct_pack::get_write_size(t.x);
}
template <typename Writer>
void sp_serialize_to(Writer& writer, const test& t) {
  struct_pack::write(writer, t.x);
}

template <typename Reader>
struct_pack::errc sp_deserialize_to(Reader& reader, test& ar) {
  return struct_pack::read(reader, ar.x);
}
constexpr std::string_view sp_set_type_name(test*) { return "myint"; }
}  // namespace my_name_space

TEST_CASE("test user-defined type name") {
  {
    auto name = struct_pack::get_type_literal<my_name_space::test>();
    std::string_view sv{name.data(), name.size()};
    CHECK(sv == (char)struct_pack::detail::type_id::user_defined_type +
                    std::string{"myint"} +
                    (char)struct_pack::detail::type_id::type_end_flag);
  }
  {
    auto name =
        struct_pack::get_type_literal<std::vector<my_name_space::test>>();
    std::string_view sv{name.data(), name.size()};
    CHECK(sv == std::string{(char)struct_pack::detail::type_id::container_t} +
                    (char)struct_pack::detail::type_id::user_defined_type +
                    std::string{"myint"} +
                    (char)struct_pack::detail::type_id::type_end_flag);
  }
}
