/*
 * Copyright (c) 2023, Alibaba Group Holding Limited;
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
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <ostream>
#include <sstream>

// It's easy to use struct_pack, just include it!
#include <ylt/struct_pack.hpp>

// 1. make sure your type has a default constructor
// 2. add marco STRUCT_PACK_REFL(Type, field1, field2...) in the same namespace
// 3. The field can be public or private, it's OK.
namespace example {
class person : std::vector<int> {
 private:
  int age;

 public:
  std::string name;
  auto operator==(const person& rhs) const {
    return age == rhs.age && name == rhs.name;
  }
  person() = default;
  person(int age, const std::string& name) : age(age), name(name) {}
};

STRUCT_PACK_REFL(person, age, name);
}  // namespace example

// 4. We also support member function in marco STRUCT_PACK_REFL(Type,
// MemFuncName1, MemFuncName2...)
// 5. Make sure the function has const version and non-const version overload.
namespace example2 {
class person {
 private:
  int age_;
  std::string name_;

 public:
  auto operator==(const person& rhs) const {
    return age_ == rhs.age_ && name_ == rhs.name_;
  }
  person() = default;
  person(int age, const std::string& name) : age_(age), name_(name) {}

  int& age() { return age_; };
  const int& age() const { return age_; };
  std::string& name() { return name_; };
  const std::string& name() const { return name_; };
};
STRUCT_PACK_REFL(person, age, name);
}  // namespace example2

// 6. you can also mix the field and function with any order in marco
// STRUCT_PACK_REFL
namespace example3 {
class person {
 private:
  int age_;
  std::string name_;
  double salary;

 public:
  auto operator==(const person& rhs) const {
    return age_ == rhs.age_ && name_ == rhs.name_ && salary == rhs.salary;
  }
  person() = default;
  person(int age, const std::string& name, double salary)
      : age_(age), name_(name) {}

  int& age() { return age_; };
  const int& age() const { return age_; };
  std::string& name() { return name_; };
  const std::string& name() const { return name_; };
};
STRUCT_PACK_REFL(person, name, age, salary);
}  // namespace example3

// 7. Remember, the STURCT_PACK_REFL marco disable the trivial_serialize
// optimize. So don't use it for trivial type.
namespace example4 {
struct point {
  int x, y, z;
};
STRUCT_PACK_REFL(point, x, y, z);
struct point2 {
  int x, y, z;
};
// The point & point2 are different type
static_assert(struct_pack::get_type_code<example4::point>() !=
              struct_pack::get_type_code<example4::point2>());
}  // namespace example4

// 8. The order of field in STRUCT_PACK_REFL will change the type of struct in
// struct_pack type system
namespace example5 {
class person {
  int age;
  std::string name;
  auto operator==(const person& rhs) const {
    return age == rhs.age && name == rhs.name;
  }
};

STRUCT_PACK_REFL(person, age, name);
class person2 {
  int age;
  std::string name;
  auto operator==(const person2& rhs) const {
    return age == rhs.age && name == rhs.name;
  }
};
STRUCT_PACK_REFL(person2, name, age);
// person & person 2 are different type
static_assert(struct_pack::get_type_code<person>() !=
              struct_pack::get_type_code<person2>());
}  // namespace example5

//clang-format off
void non_aggregated_type() {
  {
    example::person p{20, "tom"};
    auto buffer = struct_pack::serialize(p);
    auto p2 = struct_pack::deserialize<example::person>(buffer);
    assert(p2);
    assert(p == p2.value());
  }
  {
    example2::person p{20, "tom"};
    auto buffer = struct_pack::serialize(p);
    auto p2 = struct_pack::deserialize<example2::person>(buffer);
    assert(p2);
    assert(p == p2.value());
  }
  {
    example3::person p{20, "tom", 114.514};
    auto buffer = struct_pack::serialize(p);
    auto p3 = struct_pack::deserialize<example2::person>(buffer);
    assert(p3);
    assert(p == p3.value());
  }
}