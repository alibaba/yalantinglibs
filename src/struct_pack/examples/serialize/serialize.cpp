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
#include <iostream>
#include <memory>

#include "struct_pack/struct_pack.hpp"

struct person {
  int age;
  std::string name;

  auto operator==(const person &rhs) const {
    return age == rhs.age && name == rhs.name;
  }
};

void basic_usage() {
  person p{20, "tom"};

  // serialize api
  {// api 1. return default container
   {auto buffer = struct_pack::serialize(p);
}
// api 2. use specifier container
{ auto buffer = struct_pack::serialize<std::string>(p); }
// api 3. serialize to container's back
{
  std::string buffer = "The next line is struct_pack data.\n";
  struct_pack::serialize_to(buffer, p);
}
// api 4. serialize to continuous buffer
{
  auto sz = struct_pack::get_needed_size(p);
  auto array = std::make_unique<char[]>(sz);
  [[maybe_unused]] auto len = struct_pack::serialize_to(array.get(), sz, p);
  assert(len == sz);
  // if buffer's size < struct_pack::get_needed_size(p), the return value is
  // zero, and the buffer won't be written.
  len = struct_pack::serialize_to(array.get(), sz - 1, p);
  assert(len == 0);
}
// api 5. serialize with offset
{
  auto buffer = struct_pack::serialize_with_offset(2, p);
  assert(buffer[0] == '\0' && buffer[1] == '\0');
}
}

// deserialize api
{
  auto buffer = struct_pack::serialize(p);
  // api 1. deserialize object to return value
  {
    auto [ec, p2] =
        struct_pack::deserialize<person>(buffer.data(), buffer.size());
    assert(ec == std::errc{});
    assert(p == p2);
  }
  // api 2. deserialize object to reference
  {
    person p2;
    [[maybe_unused]] auto ec = struct_pack::deserialize_to(p2, buffer.data(), buffer.size());
    assert(ec == std::errc{});
    assert(p == p2);
  }
  // api 3. partial deserialize
  {
    auto [err, name] =
        struct_pack::get_field<person, 1>(buffer.data(), buffer.size());
    assert(p.name == name);
  }
}
}

int main() { basic_usage(); }
