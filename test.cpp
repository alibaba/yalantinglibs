#include <ylt/struct_pack.hpp>
struct test3 {
 public:
  test3() : x(0.f), y(0.f) {}

 public:
  float x, y;
};

struct test {
 public:
  test() : x(0.f), y(0.f) {}

  test(test3 tr3) : x(tr3.x), y(tr3.y) {}

 public:
  float x, y;
};
STRUCT_PACK_REFL(test, x, y)

struct test2 {
  test xxx;
};
STRUCT_PACK_REFL(test2, xxx)
constexpr auto i=struct_pack::members_count<test2>;
static_assert(i==1);
int main() {
  test2 ttt;
  auto data = struct_pack::serialize(ttt);
}
