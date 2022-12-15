#include <cassert>
#include <cstdint>
#include <fstream>
#include <ios>
#include <iostream>
#include <string>
#include <type_traits>

#include "struct_json/json_reader.h"
#include "struct_json/json_writer.h"
#include "struct_pack/struct_pack.hpp"
#include "struct_pack/struct_pack/struct_pack_impl.hpp"

struct person {
  std::string name;
  int age;
};

bool operator==(const person& a, const person& b) {
  return a.name == b.name && a.age == b.age;
}

REFLECTION(person, name, age);

struct field_t {
  std::string field_name;
  std::string field_type;
};
REFLECTION(field_t, field_name, field_type);

struct json_struct {
  uint32_t hash_code;
  std::string struct_name;
  std::vector<field_t> fileds;
};
REFLECTION(json_struct, hash_code, struct_name, fileds);

std::string id_to_string(struct_pack::detail::type_id id) {
  using namespace struct_pack::detail;
  switch (id) {
    case type_id::int32_t:
      return "int32";
    case type_id::uint32_t:
      return "uint32";
    case type_id::string_t:
      return "string";
    default:
      throw std::invalid_argument("unknown type id");
  }
}

template <typename T>
void gen_meta_info() {
  uint32_t hash_code =
      struct_pack::detail::packer<char, T>::template calculate_raw_hash<T>();

  T p{};
  json_struct js;
  js.hash_code = hash_code;
  iguana::for_each(p, [&js](auto filed, auto I) {
    js.struct_name = iguana::get_name<decltype(p)>();

    field_t fld{};
    auto str = iguana::get_name<decltype(p), decltype(I)::value>();
    fld.field_name = {str.data(), str.size()};
    using field_type = std::remove_cvref_t<decltype(p.*filed)>;
    constexpr auto id = struct_pack::detail::get_type_id<field_type>();
    fld.field_type = id_to_string(id);
    js.fileds.push_back(fld);
  });

  std::string json_str;
  struct_json::to_json(js, json_str);
  auto pretty_str = struct_json::prettify(json_str);
  std::cout << pretty_str << "\n";
  std::ofstream out(js.struct_name + "_meta.json", std::ios::binary);
  out.write(pretty_str.data(), pretty_str.size());
  out.flush();
  out.close();
}

int main() {
  gen_meta_info<person>();
  person p{.name = "tom", .age = 20};
  std::string str;

  // struct to json
  struct_json::to_json(p, str);
  std::cout << str << "\n";

  person p1;
  // json to struct
  struct_json::from_json(p1, str);
  assert(p == p1);

  // dom parse, json to dom
  struct_json::jvalue val;
  struct_json::parse(val, str);
  assert(val.at<std::string>("name") == "tom");
  assert(val.at<int>("age") == 20);
}