# struct_pack 使用提示
本文档介绍一些struct_pack的细节，包括一些优化和约束。
## 约束条件

struct_pack需要你的编译器良好的支持C++17标准。我们已在GCC/Clang/MSVC编译器下进行了测试

| 编译器      | 已测试的最低版本    | 
| ----------- | ------------------   |
| GCC         | 9.1                               |
| Clang       | 6.0                               |
| MSVC        | Visual Studio 2019 (MSVC19.20)    |

struct_pack是跨平台的。它可以在小端/大端，32位/64位架构下正常工作。

当启用C++20标准时，struct_pack具有更好的性能，能支持自定义的内存连续的可平凡拷贝容器。
而在C++17标准下，struct_pack仅支持标准库中已有的`std::array<T>`, `std::vector<T>`, `std::basic_string<T>` 和 `std::basic_string_view<T>`，不支持其他自定义容器类型的平凡拷贝。

## 端序

struct_pack即使在大端架构下，也会将数据保存为小端格式。当对象的宽度大于1字节时，平凡复制优化和零复制优化将被禁用。因此，struct_pack在小端架构中的性能优于大端架构。

## 宏
| 宏      | 作用 |
| ----------- | ------------------ |
| STRUCT_PACK_OPTIMIZE               |增加模板实例化的数量，通过牺牲编译时间和二进制体积来换取更好的性能    |
|STRUCT_PACK_ENABLE_UNPORTABLE_TYPE |允许序列化unportable的类型，如wchar_t和wstring，请注意，这些类型序列化出的二进制数据可能无法正常的在其他平台下反序列化|
| STRUCT_PACK_ENABLE_INT128 | 允许序列化128位整数，包括__uint128和__int128类型。请注意，只有部分编译器在部分架构下支持该类型。
## 如何让序列化or反序列化更加高效
1. 考虑使用string_view代替string, 使用span代替vector/array；
2. 把平凡字段封装到一个单独的结构体中，优化拷贝速度；
3. 尽量使用内存连续的容器(std::vector)；
4. 定义宏`STRUCT_PACK_OPTIMIZE`, 允许更充分的代码展开，通过牺牲编译时间和可执行文件大小的方式来加速。
##  类型校验
struct_pack在编译期会根据被序列化的对象的类型生成类型字符串，并根据该字符串生成一个32位的MD5并取其高31位作为类型校验码。反序列化时会检查校验码是否和待反序列化的类型相同。
为了缓解可能出现的哈希冲突，在debug模式下，struct_pack会存储完整的类型字符串。因此debug模式下生成的二进制体积比release略大。
##  序列化设置
struct_pack允许通过`struct_pack::sp_config`来配置序列化生成的元数据内容，目前的设置项有以下几个：
| 枚举值      | 作用 |
| ----------- | ------------------ |
| DEFAULT               |默认值          |
| DISABLE_TYPE_INFO  |禁止在序列化数据中存储完整类型信息，即使在DEBUG模式下|
| ENABLE_TYPE_INFO | 强制在序列化数据中存储完整类型信息，即使不在DEBUG模式下|
| DISABLE_META_INFO| 禁用元信息和4字节的类型校验码，从而减小二进制数据的体积|
需要注意的是，当序列化配置了DISABLE_META_INFO选项时，必须保证反序列化也使用了该选项，否则行为未定义，大概率反序列化会失败。

假设有类型：
```cpp
struct rect {
  var_int a, b, c, d;
};
```

### 使用ADL作为序列化配置

在类型`rect`相同的namespace 下添加函数`set_sp_config`即可。

```cpp
inline constexpr struct_pack::sp_config set_sp_config(rect*) {
  return struct_pack::DISABLE_ALL_META_INFO;
}
```

### 使用模板参数配置

该方法要求每次序列化时都将配置作为模板参数传入：

```cpp
auto buffer = struct_pack::serialize<struct_pack::DISABLE_ALL_META_INFO>(rect{});
auto result = struct_pack::deserialize<struct_pack::DISABLE_ALL_META_INFO,rect>(buffer);
```

当同时配置了两者时，模板参数的优先级高于ADL。      
目前我们不允许在具有compatible字段的情况下启用`DISABLE_ALL_META_INFO`配置。

##  类型约束

1. 序列化的类型必须是struct_pack类型系统中的合法类型。详见：struct_pack的类型系统。See document：[struct_pack 类型系统](https://alibaba.github.io/yalantinglibs/zh/struct_pack/struct_pack_type_system.html)。
2. struct_pack允许新增struct_pack::compatible字段，并保证其前向/后向的兼容性，只要每次协议变更时填入的版本号大于上一次的版本号即可。如果删除/修改了已有字段，则无法保证兼容性。详见[文档](https://alibaba.github.io/yalantinglibs/zh/struct_pack/struct_pack_type_system.html#%E5%85%BC%E5%AE%B9%E7%B1%BB%E5%9E%8B)

