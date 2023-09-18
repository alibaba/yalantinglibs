#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "doctest.h"
#include "ylt/struct_pack.hpp"
#include "ylt/struct_pack/struct_pack_impl.hpp"
struct Base {
  Base(){};
  static struct_pack::expected<std::unique_ptr<Base>, struct_pack::errc>
  deserialize(std::string_view sv);
  virtual std::string get_name() const = 0;
  virtual ~Base(){};
};
struct foo : public Base {
  std::string hello = "1";
  std::string hi = "2";
  virtual std::string get_name() const override { return "foo"; }
  friend bool operator==(const foo& a, const foo& b) {
    return a.hello == b.hello && a.hi == b.hi;
  }
};
STRUCT_PACK_REFL(foo, hello, hi);
struct foo2 : public Base {
  std::string hello = "1";
  std::string hi = "2";
  virtual std::string get_name() const override { return "foo2"; }
  friend bool operator==(const foo2& a, const foo2& b) {
    return a.hello == b.hello && a.hi == b.hi;
  }
  static constexpr uint64_t struct_pack_id = 114514;
};
STRUCT_PACK_REFL(foo2, hello, hi);
struct foo3 : public Base {
  std::string hello = "1";
  std::string hi = "2";
  virtual std::string get_name() const override { return "foo3"; }
  friend bool operator==(const foo3& a, const foo3& b) {
    return a.hello == b.hello && a.hi == b.hi;
  }
};
STRUCT_PACK_REFL(foo3, hello, hi);
struct foo4 : public Base {
  std::string hello = "1";
  std::string hi = "2";
  virtual std::string get_name() const override { return "foo4"; }
  friend bool operator==(const foo4& a, const foo4& b) {
    return a.hello == b.hello && a.hi == b.hi;
  }
};
constexpr int struct_pack_id(foo4*) { return 112233211; }
STRUCT_PACK_REFL(foo4, hello, hi);
struct bar : public Base {
  int oh = 1;
  int no = 2;
  virtual std::string get_name() const override { return "bar"; }
  friend bool operator==(const bar& a, const bar& b) {
    return a.oh == b.oh && a.no == b.no;
  }
};
STRUCT_PACK_REFL(bar, oh, no);
struct gua : Base {
  std::string a = "Hello";
  int b = 1;
  virtual std::string get_name() const override { return "gua"; }
  friend bool operator==(const gua& a, const gua& b) {
    return a.a == b.a && a.b == b.b;
  }
};
STRUCT_PACK_REFL(gua, a, b);

struct_pack::expected<std::unique_ptr<Base>, struct_pack::errc>
Base::deserialize(std::string_view sv) {
  return struct_pack::deserialize_derived_class<Base, bar, foo, gua, foo2,
                                                foo4>(sv);
}

TEST_CASE("testing derived") {
  using namespace std::string_literals;
  static_assert(struct_pack::detail::has_user_defined_id_ADL<foo4>);
  foo f;
  foo2 f2;
  foo4 f4;
  bar b;
  gua g;
  std::vector vecs{struct_pack::serialize<std::string>(f),
                   struct_pack::serialize<std::string>(b),
                   struct_pack::serialize<std::string>(g),
                   struct_pack::serialize<std::string>(f2),
                   struct_pack::serialize<std::string>(f4)};
  auto f1 = Base::deserialize(vecs[0]);
  auto b1 = Base::deserialize(vecs[1]);
  auto g1 = Base::deserialize(vecs[2]);
  auto f21 = Base::deserialize(vecs[3]);
  auto f41 = Base::deserialize(vecs[4]);
  CHECK(*(foo*)(f1->get()) == f);
  CHECK(*(foo2*)(f21->get()) == f2);
  CHECK(*(bar*)(b1->get()) == b);
  CHECK(*(gua*)(g1->get()) == g);
  std::vector<std::pair<std::unique_ptr<Base>, std::string>> vec;
  vec.emplace_back(std::move(f1.value()), "foo");
  vec.emplace_back(std::move(f21.value()), "foo2");
  vec.emplace_back(std::move(b1.value()), "bar");
  vec.emplace_back(std::move(g1.value()), "gua");
  vec.emplace_back(std::move(f41.value()), "foo4");
  for (const auto& e : vec) {
    CHECK(e.first->get_name() == e.second);
  }
}

TEST_CASE("test hash collision") {
  static_assert(struct_pack::detail::MD5_set<foo, bar, foo2, gua, foo3,
                                             foo4>::has_hash_collision != 0);
}