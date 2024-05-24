# struct_pb 简介

struct_pb 是基于C++17 开发的高性能、易用、header only的protobuf格式序列化/反序列化库。

## 动机
不再依赖proto文件去定义dsl message，而是通过C++ 结构体去定义需要序列化/反序列化的对象；因为没有protoc文件所以也不再依赖protoc去生成代码。通过C++17去实现可以做很多性能优化，从而获得更好的性能，比如可以支持反序列化时对字符串的零拷贝、尽可能内联和编译期计算以及字符串非memset的resize等。

## 例子

### 定义结构体
```cpp
#include <ylt/struct_pb.hpp>

struct my_struct : struct_pb::pb_base_impl<my_struct> {
  int x;
  bool y;
  struct_pb::fixed64_t z;
};
REFLECTION(my_struct, x, y, z);

struct nest : struct_pb::pb_base_impl<nest> {
  std::string name;
  my_struct value;
  int var;
};
REFLECTION(nest, name, value, var);
```

### 序列化
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
上面的这个结构体如果对应到protobuf的proto文件则是:
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
在benchmark monster场景下，struct_pb 性能比protobuf 更好，序列化速度是protobuf的2.4倍，反序列化是protobuf的3.4倍。详情可以自行运行struct_pack 中的benchmark复现结果。

## struct_pb 和 protobuf 类型映射
Scalar Value Types with no modifier (a.k.a **singular**) -> T

Scalar Value Types with **optional** -> `std::optional <T>`

any type with **repeat** -> `std::vector<T>`

types with **map** -> `std::map<K, V>`

any message type -> `std::optional <T>`

enum -> enum class

oneof -> `std::variant <...>`

### 映射表
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

## 约束
- 目前还只支持proto3，不支持proto2；
- 目前还没支持反射；
- 还没支持unkonwn字段；
- struct_pb 结构体必须派生于pb_base_impl

## roadmap
- 支持proto2；
- 支持反射；
- 支持unkonwn字段；
- 去除struct_pb 结构体必须派生于pb_base_impl的约束；
