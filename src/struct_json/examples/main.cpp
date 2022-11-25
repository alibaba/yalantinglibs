#include <cassert>
#include <iostream>

#include "struct_json/json_reader.h"
#include "struct_json/json_writer.h"

struct person {
  std::string name;
  int age;
};

bool operator==(const person& a, const person& b) {
  return a.name == b.name && a.age == b.age;
}

REFLECTION(person, name, age);

int main() {
  person p{.name = "tom", .age = 20};
  std::string str;
  struct_json::to_json(p, str);
  std::cout << str << "\n";

  person p1;
  struct_json::from_json(p1, str);
  assert(p == p1);
}