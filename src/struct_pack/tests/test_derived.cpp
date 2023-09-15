#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "doctest.h"
#include "ylt/struct_pack.hpp"
struct Base {
  static struct_pack::expected<std::shared_ptr<Base>, struct_pack::errc>
  deserialize(std::string_view sv);
};
struct foo : public Base {
  std::string hello;
  std::string hi;
  friend bool operator==(const foo& a, const foo& b) {
    return a.hello == b.hello && a.hi == b.hi;
  }
};
struct bar : public Base {
  int oh;
  int no;
  friend bool operator==(const bar& a, const bar& b) {
    return a.oh == b.oh && a.no == b.no;
  }
};
struct gua : Base {
  std::vector<foo> foos;
  std::vector<bar> bars;
  friend bool operator==(const gua& a, const gua& b) {
    return a.foos == b.foos && a.bars == b.bars;
  }
};
struct_pack::expected<std::shared_ptr<Base>, struct_pack::errc>
Base::deserialize(std::string_view sv) {
  return struct_pack::deserialize_derived_class<Base, foo, bar, gua>(sv);
}
TEST_CASE("testing derived") {
  foo f{.hello = "1", .hi = "2"};
  bar b{.oh = 1, .no = 2};
  gua g{.foos = {{.hello = "23", .hi = "34"}, {.hello = "45", .hi = "67"}},
        .bars = {{.oh = 1, .no = 2}, {.oh = 3, .no = 4}}};
  std::vector vecs{struct_pack::serialize<std::string>(f),
                   struct_pack::serialize<std::string>(b),
                   struct_pack::serialize<std::string>(g)};
  auto f1 = Base::deserialize(vecs[0]);
  auto b1 = Base::deserialize(vecs[1]);
  auto g1 = Base::deserialize(vecs[2]);
  CHECK(*(foo*)(f1->get()) == f);
  CHECK(*(bar*)(b1->get()) == b);
  CHECK(*(gua*)(g1->get()) == g);
}