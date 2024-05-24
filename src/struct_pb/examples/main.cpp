#include <cassert>
#include <iostream>
#include <ylt/struct_pb.hpp>

struct my_struct : struct_pb::pb_base_impl<my_struct> {
  my_struct() = default;
  my_struct(int a, bool b, struct_pb::fixed64_t c) : x(a), y(b), z(c) {}
  int x;
  bool y;
  struct_pb::fixed64_t z;
};
REFLECTION(my_struct, x, y, z);

struct nest : struct_pb::pb_base_impl<nest> {
  nest() = default;
  nest(std::string s, my_struct t, int v)
      : name(std::move(s)), value(t), var(v) {}
  std::string name;
  my_struct value;
  int var;
};
REFLECTION(nest, name, value, var);

int main() {
  nest v{"Hi", my_struct{1, false, {3}}, 5};
  std::string s;
  struct_pb::to_pb(v, s);

  nest v2;
  struct_pb::from_pb(v2, s);
  assert(v.var == v2.var);
  assert(v.value.y == v2.value.y);
  assert(v.value.z == v2.value.z);
}