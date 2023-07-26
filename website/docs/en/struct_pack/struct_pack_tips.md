# struct_pack Tips
This document introcude some tips about struct_pack
## compiler requires
| compiler      | min support version |
| ----------- | ------------------ |
| GCC         | 10.1               |
| Clang       | 10.0.0             |
| MSVC        | 19.29(VS 16.11)    |
## macro
| macro      | description |
| ----------- | ------------------ |
| STRUCT_PACK_OPTIMIZE               | Allow more extremed loop unrolling to get better performance, but it cost more compile time.    |
| STRUCT_PACK_ENABLE_UNPORTABLE_TYPE | Enable serialize unportable type, such as wchar_t and wstring. Deserialize them in other platform is ubdefined bevaior. |
## How to speed up serialization/deserialization
1. use string_view instead of string, use span instead of vector/array.
2. move trivial field to a standlone struct, so that struct_pack could copy it faster. 
3. use std::vector instead of std::list.
4. define macro `STRUCT_PACK_OPTIMIZE`.
## Type Check
struct_pack will generate a name for the serialized type during compilation, and get a 32-bit MD5 based on the string, then take its upper 31 bits for type check. When deserializing, it will check whether the hash code stored is the same as the type to be deserialized. 
In order to alleviate possible hash collisions, in debug mode, struct_pack will store the complete type name instead of hash code. Therefore, the binary size in debug mode is slightly larger than release.
## Type requires

1. The type to serialize should be legal struct_pack type.。See document：[struct_pack type system](https://alibaba.github.io/yalantinglibs/en/struct_pack/struct_pack_type_system.html)。
2. struct_pack support update protocol by add struct_pack::compatible field, which is forward backward compatibility. User should make sure the version number is increment for each update. It's not allow to delete/modify exist field. See : [document](https://alibaba.github.io/yalantinglibs/en/struct_pack/struct_pack_type_system.html#%E5%85%BC%E5%AE%B9%E7%B1%BB%E5%9E%8B)

