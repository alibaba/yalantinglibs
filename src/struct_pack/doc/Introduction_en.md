# struct_pack Introduction

[TOC]

- [struct_pack Introduction](#struct_pack-introduction)
  - [Serialization](#serialization)
    - [Basic Usage](#basic-usage)
    - [Explicit data container](#explicit-data-container)
    - [Append the result at the end of existing data container](#append-the-result-at-the-end-of-existing-data-container)
    - [Save the results to memory location indicated by pointer](#save-the-results-to-memory-location-indicated-by-pointer)
    - [Multi-parameter serialization](#multi-parameter-serialization)
  - [Deserialization](#deserialization)
    - [Basic Usage](#basic-usage)
    - [deserialize from pointers](#deserialize-from-pointers)
    - [deserialize to an existing object](#deserialize-to-an-existing-object)
    - [Multi-parameter deserialization](#multi-parameter-deserialization)
    - [Partial deserialization](#partial-deserialization)
  - [support std containers, std::optional and custom containers](#support-std-containers-stdoptional-and-custom-containers)
  - [benchmark](#benchmark)
    - [Test case](#test-case)
    - [Test objects](#test-objects)
    - [Test Environment](#test-environment)
    - [Test results](#test-results)
  - [Forward/backward compatibility](#forwardbackward-compatibility)
  - [Why is struct_pack faster?](#why-is-struct_pack-faster)
  - [Appendix](#appendix)
    - [Test code](#test-code)

struct_pack is a serialization library featuring zero-cost abstraction as well as usability. In struct_pack, the serialization or deserialization of one complex structure could easily be done in a single line of code, without any DSL, macro, or template to be defined. struct_pack supports the serialization of C++ structures through compile-time reflection and its performance is significantly better than protobuf and msgpack (see the benchmark section for details).

Below, we show the basic usage of struct_pack with a simple object as an example.

```cpp
struct person {
  int64_t id;
  std::string name;
  int age;
  double salary;
};

person person1{.id = 1, .name = "hello struct pack", .age = 20, .salary = 1024.42};
```

## Serialization

In below we demonstrate serval ways of serialize one object with struct_pack APIs.

### Basic Usage 

```cpp
// serialization in one line
std::vector<char> result = struct_pack::serialize(person1);
```

### Explicit data container

```cpp
auto result = struct_pack::serialize<std::string>(person1);
// explicitly use std::string instead of std::vector<char> to hold the result
```

### Append the result at the end of existing data container

```cpp
std::string result="The next line is struct_pack serialize result.\n";
auto result = struct_pack::serialize_to(result,person1);
// 
```

### Save the results to memory location indicated by pointer

```cpp
auto sz=struct_pack::get_needed_siarray(person1);
std::unique_ptr array=std::make_unique<char[]>(sz);
auto result = struct_pack::serialize_to(array.get(),sz,person1);
// save the result at given memory location by pointer
```

### Multi-parameter serialization

```cpp
auto result=struct_pack::serialize(person1.id, person1.name, person1.age, person1.salary);
//serialize as std::tuple<int64_t, std::string, int, double>
```

## Deserialization

In below we demonstrate serval ways of deserialize one object with struct_pack APIs.

### Basic Usage

```cpp
// deserialize in one line
auto person2 = deserialize<person>(buffer);
assert(person2); // person2.has_value() == true
assert(person2.value()==person1);
```

### deserialize from pointers

```cpp
// deserialize from memory location indicated by pointers
auto person2 = deserialize<person>(buffer.data(),buffer.size());
assert(person2); //person2.has_value() == true
assert(person2.value()==person1);
```

### deserialize to an existing object

```cpp
// deserialize to an existing object
person person2;
std::errc ec = deserialize_to(person2, buffer);
assert(ec==std::errc{}); // person2.has_value() == true
assert(person2==person1);
```

### Multi-parameter deserialization

```cpp
auto person2 = deserialize<int64_t,std::string,int,double>(buffer);
assert(person2); // person2.has_value() == true
auto &&[id,name,age,salary]=person2.value();
assert(person1.id==id);
assert(person1.name==name);
assert(person1.age==age);
assert(person1.salary==salary);
```


### Partial deserialization

Sometimes users only need to deserialize specific fields of an object instead of all of them, and that's when the partial deserialization feature can be used. This can avoid full deserialization and improve efficiency significantly, eg.:

```cpp
// Just deserialize the 2nd field of person 
auto name = get_field<person, 1>(buffer.data(), buffer.size());
assert(name); // name.has_value() == true
assert(name.value() == "hello struct pack");
```

## support std containers, std::optional and custom containers

For example, the library supports the following complicated objects with std containers and std::optional fields:

```cpp
enum class Color { red, black, white };

struct complicated_object {
  Color color;
  int a;
  std::string b;
  std::vector<person> c;
  std::list<std::string> d;
  std::deque<int> e;
  std::map<int, person> f;
  std::multimap<int, person> g;
  std::set<std::string> h;
  std::multiset<int> i;
  std::unordered_map<int, person> j;
  std::unordered_multimap<int, int> k;
  std::array<person, 2> m;
  person n[2];
  std::pair<std::string, person> o;
  std::optional<int> p;
};

struct nested_object {
  int id;
  std::string name;
  person p;
  complicated_object o;
};

nested_object nested{.id = 2, .name = "tom", .p = {20, "tom"}, .o = {}};
auto buffer = serialize(nested);
auto nested2 = deserialize(buffer.data(), buffer.size());
assert(nested2)
assert(nested2==nested1);
```

In addition, struct_pack supports serialization and deserialization on custom containers, as below:

```cpp
// We should not inherit from stl container, this case just for testing.
template <typename Key, typename Value>
struct my_map : public std::map<Key, Value> {};

my_map<int, std::string> map1;
map1.emplace(1, "tom");
map1.emplace(2, "jerry");

absl::flat_hash_map<int, std::string> map2 =
    {{1, "huey"}, {2, "dewey"}, {3, "louie"},};

auto buffer1 = serialize(map1);
auto buffer2 = serialize(map2);
```

## benchmark

### Test case

The object to be serialized is pre-initialized and the memory to store the serialization result is pre-allocated. For each test case, we run one million serializations/deserialization and take the average.

### Test objects

1. A simple object `person` with 4 scaler types

```cpp
struct person {
  int64_t id;
  std::string name;
  int age;
  double salary;
};
```

2. A complicated object `monster` with nested types

```cpp
enum Color : uint8_t { Red, Green, Blue };

struct Vec3 {
  float x;
  float y;
  float z;
};

struct Weapon {
  std::string name;
  int16_t damage;
};

struct Monster {
  Vec3 pos;
  int16_t mana;
  int16_t hp;
  std::string name;
  std::vector<uint8_t> inventory;
  Color color;
  std::vector<Weapon> weapons;
  Weapon equipped;
  std::vector<Vec3> path;
};
```

3. A `rect` object with for `int32_t`

```cpp
struct rect {
  int32_t x;
  int32_t y;
  int32_t width;
  int32_t height;
};
```

### Test Environment

Compiler: Alibaba Clang 13

Processor: (Intel(R) Xeon(R) Platinum 8163 CPU @ 2.50GHz)

### Test results

![](./images/struct_pack_bench.png)

## Forward/backward compatibility

If current message type no longer meets all you needs - say, you'd like the object to  have an extra field, the compatibility should not be broken so that the old object could be correctly parsed with the new type definition. In struct_pack, any new fields you added must be of type `struct_pack::compatible<T>` and be appended **at the end of the object**. <br />Let's take struct `person` as an example:

```cpp
struct person {
  int age;
  std::string name;
};

struct person1 {
  int age;
  std::string name;
  struct_pack::compatible<int32_t> id;
  struct_pack::compatible<bool> maybe;
};
```

struct_pack ensures that the two classes can be safely converted to each other by serialization and deserialization, thus achieving forward/backward compatibility.

## Why is struct_pack faster?

1. Streamlined type information and efficient type-checking. MD5 computation is done at compile time so at runtime we only needs to compare 32bit hash values to check sameness.
2. struct_pack is a template library that encourages the compiler to do a better job at inlining.
4. Zero-cost abstraction, no runtime cost for features that are not used.
5. struct_pack's memory layout is much closer to the native C++ object, reducing the workload of (de)serializations.
6. Compile-time type calculation allows struct_pack to generate different codes according to different types. So we can do optimization upon types. For example, we could use `memcpy` on contiguous containers and zero-copy optimization could be used on `string_view`

## Appendix

### Test code

see [benchmark.cpp](../benchmark/benchmark.cpp)
