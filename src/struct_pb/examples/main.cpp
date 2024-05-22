#include <cassert>
#include <iostream>
#include <ylt/struct_pb.hpp>

struct my_struct : struct_pb::pb_base {
  int x;
  bool y;
  struct_pb::fixed64_t z;
};
REFLECTION(my_struct, x, y, z);

struct nest : struct_pb::pb_base {
  std::string name;
  my_struct value;
  int var;
};
REFLECTION(nest, name, value, var);

int main() {
  nest v{0, "Hi", {0, 1, false, 3}, 5}, v2{};
  std::string s;
  struct_pb::to_pb(v, s);
  struct_pb::from_pb(v2, s);
  assert(v.var == v2.var);
  assert(v.value.y == v2.value.y);
  assert(v.value.z == v2.value.z);
}