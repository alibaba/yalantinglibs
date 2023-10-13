# struct_pack Tips
This document introcude some tips about struct_pack
## requirements

struct_pack need compiler at least support C++17. We have test it on GCC/Clang/MSVC.

| compiler      | tested min version    | 
| ----------- | ------------------   |
| GCC         | 9.1                               |
| Clang/Apple Clang   | 6.0                        |
| MSVC        | Visual Studio 2019 (MSVC19.20)    |

struct_pack is cross-platform. struct_pack can work in both big-endian and little-endian. struct_pack support 32bit arch or 64bit arch. 

When C++20 standard was enabled, struct_pack has better performance by support memcpy user-defined trivial continuous container. In C++17, struct_pack only support memcpy container `std::array<T>`, `std::vector<T>`, `std::basic_string<T>` and `std::basic_string_view<T>`.

## endian

struct_pack will save the data as little-endian even if the arch is big-endian. The trivial-copy optimize and zero-copy optimize will be disable when the object is width than 1 byte. so struct_pack performance in little-endian is better than big-endian.

## macro
| macro      | description |
| ----------- | ------------------ |
| STRUCT_PACK_OPTIMIZE               | Allow more extremed loop unrolling to get better performance, but it cost more compile time.    |
| STRUCT_PACK_ENABLE_UNPORTABLE_TYPE | Enable serialize unportable type, such as wchar_t and wstring. Deserialize them in other platform is ubdefined bevaior. |
| STRUCT_PACK_ENABLE_INT128 | Enable serialize __int128 and __uint128. Not all compiler support it.
## How to speed up serialization/deserialization
1. use string_view instead of string, use span instead of vector/array.
2. move trivial field to a standlone struct, so that struct_pack could copy it faster. 
3. use std::vector instead of std::list.
4. define macro `STRUCT_PACK_OPTIMIZE`.
## Type Check
struct_pack will generate a name for the serialized type during compilation, and get a 32-bit MD5 based on the string, then take its upper 31 bits for type check. When deserializing, it will check whether the hash code stored is the same as the type to be deserialized. 
In order to alleviate possible hash collisions, in debug mode, struct_pack will store the complete type name instead of hash code. Therefore, the binary size in debug mode is slightly larger than release. User can also enable it manually by add option `struct_pack::type_info_config::enable`:
```cpp
auto result=struct_pack::serialize<struct_pack::type_info_config::enable>(person);
```
## Type requires

1. The type to serialize should be legal struct_pack type.。See document：[struct_pack type system](https://alibaba.github.io/yalantinglibs/en/struct_pack/struct_pack_type_system.html)。
2. struct_pack support update protocol by add struct_pack::compatible field, which is forward backward compatibility. User should make sure the version number is increment for each update. It's not allow to delete/modify exist field. See : [document](https://alibaba.github.io/yalantinglibs/en/struct_pack/struct_pack_type_system.html#%E5%85%BC%E5%AE%B9%E7%B1%BB%E5%9E%8B)

