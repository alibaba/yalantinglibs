#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <memory>

#include "struct_pack/struct_pack.hpp"
#include "struct_pack/struct_pack/struct_pack_impl.hpp"

struct person {
  int age;
  std::string name;
};

int main()
{ 
  std::fstream file;
  file.open("./golang_serialize.txt", std::ios::in | std::ios::binary);
  std::string buffer;
  file >> buffer;
  file.close();

  person p{.age = 21, .name = "Betty"};
  person p2;
  [[maybe_unused]] auto ec = struct_pack::deserialize_to(p2, buffer);
  assert(ec == struct_pack::errc{});
  assert(p == p2);

  std::cout << p2.age << std::endl;
  std::cout << p2.name << std::endl;


  return 0;
}