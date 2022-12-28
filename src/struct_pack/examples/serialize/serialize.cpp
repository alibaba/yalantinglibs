/*
 * Copyright (c) 2022, Alibaba Group Holding Limited;
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <ostream>

#include "struct_pack/struct_pack.hpp"
#include "struct_pack/struct_pack/struct_pack_impl.hpp"

struct person {
  int age;
  std::string name;

  auto operator==(const person &rhs) const {
    return age == rhs.age && name == rhs.name;
  }
};

void basic_usage() {
  person p{20, "tom"};

  person p2{.age = 21, .name = "Betty"};

  // serialize api
  // api 1. return default container

  { auto buffer = struct_pack::serialize(p); }
  // api 2. use specifier container
  { auto buffer = struct_pack::serialize<std::string>(p); }
  // api 3. serialize to container's back
  {
    std::string buffer = "The next line is struct_pack data.\n";
    struct_pack::serialize_to(buffer, p);
  }
  // api 4. serialize to continuous buffer
  {
    auto info = struct_pack::get_serialize_info(p);
    auto array = std::make_unique<char[]>(info.size());
    struct_pack::serialize_to(array.get(), info, p);
  }
  // api 5. serialize with offset
  {
    auto buffer = struct_pack::serialize_with_offset(2, p);
    assert(buffer[0] == '\0' && buffer[1] == '\0');
  }
  // api 6. serialize varadic param
  {
    person p2{.age = 21, .name = "Betty"};
    auto buffer = struct_pack::serialize(p.age, p2.name);
  }
  // api 7. serialize to stream
  {
    std::ofstream writer("test.out");
    struct_pack::serialize_to(writer, p);
  }

  // deserialize api
  auto buffer = struct_pack::serialize(p);
  // api 1. deserialize object to return value
  {
    auto p2 = struct_pack::deserialize<person>(buffer);
    assert(p2);  // no error
    assert(p2.value() == p2);
  }
  // api 2. deserialize object to reference
  {
    person p2;
    [[maybe_unused]] auto ec = struct_pack::deserialize_to(p2, buffer);
    assert(ec == struct_pack::errc{});
    assert(p == p2);
  }
  // api 3. partial deserialize
  {
    auto name = struct_pack::get_field<person, 1>(buffer);
    assert(name);  // no error
    assert(p.name == name.value());
  }
  // api 4. varadic deserialize
  {
    auto buffer = struct_pack::serialize(p.age, p2.name);
    auto result = struct_pack::deserialize<int, std::string>(buffer);
    assert(result);  // no error
    [[maybe_unused]] auto &&[age, name] = result.value();
    assert(age == p.age);
    assert(name == p2.name);
  }
  // api 4. varadic deserialize_to
  {
    person p3;
    auto buffer = struct_pack::serialize(p.age, p2.name);
    [[maybe_unused]] auto result =
        struct_pack::deserialize_to(p3.age, buffer, p3.name);
    assert(result == struct_pack::errc{});
    assert(p3.age == p.age);
    assert(p3.name == p2.name);
  }
}

int main() { basic_usage(); }
