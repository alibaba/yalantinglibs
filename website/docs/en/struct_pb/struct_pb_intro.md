# struct_pb introduction

struct_pb is an easy to use, C++17, header only, high performance protobuf format serialization library.

## motiviation

Don't depend on proto files, don't depend on protoc to generate code.

Utilize inline, zero copy, compile-time compute to optimize performance.

## example

### define struct
```cpp
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
```

### serialization and deserialization
```cpp
int main() {
  nest v{0, "Hi", {0, 1, false, 3}, 5}, v2{};
  std::string s;
  struct_pb::to_pb(v, s);
  struct_pb::from_pb(v2, s);
  assert(v.var == v2.var);
  assert(v.value.y == v2.value.y);
  assert(v.value.z == v2.value.z);
}
```
the above struct mapping to a proto file, like this:
```
message my_struct {
  int32 optional_int32 = 1;
  bool optional_int64 = 2;
  sfixed64 z = 3;
}

message nest {
  string name = 1;
  my_struct value = 2;
  int32 var = 3;
}
```

## benchmark 

monster case: 

struct_pb is 2.4 faster than protobuf when serializing a monster;

struct_pb is 3.4 faster than protobuf when deserializing a monster;

the code in struct_pack benchmark.

## struct_pb and protobuf type mapping
Scalar Value Types with no modifier (a.k.a **singular**) -> T

Scalar Value Types with **optional** -> `std::optional <T>`

any type with **repeat** -> `std::vector<T>`

types with **map** -> `std::map<K, V>`

any message type -> `std::optional <T>`

enum -> enum class

oneof -> `std::variant <...>`

### type mapping table
| .proto Type | struct_pb Type                    | pb native C++ type | Notes                              |
|-------------|-----------------------------------|--------------------|------------------------------------|
| double      | double                            | double             | 8 bytes                            |
| float       | float                             | float              | 4 bytes                            |
| int32       | int32                             | int32              | Uses variable-length encoding.     |
| int64       | int64                             | int64              |                                    |
| uint32      | uint32                            | uint32             |                                    |
| uint64      | uint64                            | uint64             |                                    |
| sint32      | sint32_t                             | int32              | ZigZag + variable-length encoding. |
| sint64      | sint6_t                             | int64              |                                    |
| fixed32     | fixed32_t                            | uint32             | 4 bytes                            |
| fixed64     | fixed64_t                            | uint64             | 8 bytes                            |
| sfixed32    | sfixed32_t                             | int32              | 4 bytes,$2^{28}$                   |
| sfixed64    | sfixed64_t                             | int64              | 8 bytes,$2^{56}$                   |
| bool        | bool                              | bool               |                                    |
| string      | std::string                       | string             | $len < 2^{32}$                     |
| bytes       | std::string                       | string             |                                    |
| enum        | enum class: int {}/enum                | enum: int {}       |                                    |
| oneof       | std::variant<...> |                    |                                    |

## limitation
- only support proto3, not support proto2 now;
- don't support reflection now;
- don't support  unkonwn fields;
- struct must inherited from pb_base;

## roadmap
- support proto2;
- support reflection;
- support unkonwn fields;
- no need inheriting from pb_base;
