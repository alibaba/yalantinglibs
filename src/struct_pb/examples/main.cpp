#include <cassert>
#include <iostream>
#include <ylt/struct_pb.hpp>

struct my_struct : struct_pb::base_impl<my_struct> {
  my_struct() = default;
  my_struct(int a, bool b, struct_pb::fixed64_t c) : x(a), y(b), z(c) {}
  int x;
  bool y;
  struct_pb::fixed64_t z;
};
REFLECTION(my_struct, x, y, z);

struct nest : struct_pb::base_impl<nest> {
  nest() = default;
  nest(std::string s, my_struct t, int v)
      : name(std::move(s)), value(t), var(v) {}
  std::string name;
  my_struct value;
  int var;
};
REFLECTION(nest, name, value, var);

struct person {
  int id;
  std::string name;
  int age;
};
REFLECTION(person, id, name, age);

int main() {
  nest v{"Hi", my_struct{1, false, {3}}, 5};
  std::string s;
  struct_pb::to_pb(v, s);

  nest v2;
  struct_pb::from_pb(v2, s);
  assert(v.var == v2.var);
  assert(v.value.y == v2.value.y);
  assert(v.value.z == v2.value.z);

  person p{1, "tom", 22};
  std::string str;
  struct_pb::to_pb(p, str);

  person p1;
  struct_pb::from_pb(p1, str);
  assert(p.age == p1.age);
  assert(p.name == p1.name);
  assert(p.id == p1.id);

  // dynamic reflection
  auto t = struct_pb::create_instance("nest");
  auto names = t->get_fields_name();
  bool r = (names == std::vector<std::string_view>{"name", "value", "var"});
  assert(r);

  t->set_field_value("name", std::string("tom"));
  auto name = t->get_field_value<std::string>("name");
  assert(name == "tom");

  auto d = dynamic_cast<nest *>(t.get());
  assert(d->name == "tom");

  t->set_field_value<std::string>("name", "hello");
  auto &field_name = t->get_field_value<std::string>("name");
  assert(field_name == "hello");
}