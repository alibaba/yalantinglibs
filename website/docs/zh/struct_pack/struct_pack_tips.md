# struct_pack 使用提示
本文档介绍一些struct_pack的细节，包括一些优化和约束。
## 编译器支持

struct_pack需要你的编译器良好的支持C++17标准。

| 编译器      | 已测试的最低版本    | 
| ----------- | ------------------   |
| GCC         | 9.1                               |
| Clang       | 6.0                               |
| MSVC        | Visual Studio 2019 (MSVC19.20)    |

当启用C++20标准时，struct_pack具有更好的性能，能支持自定义的内存连续的可平凡拷贝容器。
而在C++17标准下，struct_pack仅支持标准库中已有的`std::array<T>`, `std::vector<T>`, `std::basic_string<T>` 和 `std::basic_string_view<T>`，不支持其他自定义容器类型的平凡拷贝。

## 宏
| 宏      | 作用 |
| ----------- | ------------------ |
| STRUCT_PACK_OPTIMIZE               |增加模板实例化的数量，通过牺牲编译时间和二进制体积来换取更好的性能    |
|STRUCT_PACK_ENABLE_UNPORTABLE_TYPE |允许序列化unportable的类型，如wchar_t和wstring，请注意，这些类型序列化出的二进制数据可能无法正常的在其他平台下反序列化|
## 如何让序列化or反序列化更加高效
1. 考虑使用string_view代替string, 使用span代替vector/array；
2. 把平凡字段封装到一个单独的结构体中，优化拷贝速度；
3. 尽量使用内存连续的容器(std::vector)；
4. 定义宏`STRUCT_PACK_OPTIMIZE`, 允许更充分的代码展开，通过牺牲编译时间和可执行文件大小的方式来加速。
##  类型校验
struct_pack在编译期会根据被序列化的对象的类型生成类型字符串，并根据该字符串生成一个32位的MD5并取其高31位作为类型校验码。反序列化时会检查校验码是否和待反序列化的类型相同。
为了缓解可能出现的哈希冲突，在debug模式下，struct_pack会存储完整的类型字符串。因此debug模式下生成的二进制体积比release略大。
##  类型约束

1. 序列化的类型必须是struct_pack类型系统中的合法类型。详见：struct_pack的类型系统。See document：[struct_pack 类型系统](https://alibaba.github.io/yalantinglibs/zh/struct_pack/struct_pack_type_system.html)。
2. struct_pack允许新增struct_pack::compatible字段，并保证其前向/后向的兼容性，只要每次协议变更时填入的版本号大于上一次的版本号即可。如果删除/修改了已有字段，则无法保证兼容性。详见[文档](https://alibaba.github.io/yalantinglibs/zh/struct_pack/struct_pack_type_system.html#%E5%85%BC%E5%AE%B9%E7%B1%BB%E5%9E%8B)

