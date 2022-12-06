# struct_pack 的编码规则与布局

**默认情况**下，struct_pack的内存布局如下图所示：TODO开头4个字节是类型哈希字段，随后是实际数据的负载，该负载长度是可变的(variable length)，由用户传入的序列化对象决定。

默认情况指：**元信息头**的各个字段均为零值（默认值），因此我们可以省略元信息来压缩长度，即：
1. struct_pack不包含compatible字段
2. struct_pack不存储完整类型信息
3. struct_pack的容器最大长度小于256。

![](./images/layout/memory_layout1.svg)

假如用户传入的序列化对象不符合**默认情况**，则struct_pack会包含**元信息**字段，此时完整的内存布局如下图所示。

![](./images/layout/memory_layout2.svg)

下面我们将详细分析各字段的布局。

# 1. 类型哈希（必选）

类型哈希位于struct_pack的头部被，一共4个字节。用于struct_pack的类型校验。

高31位：类型哈希数据。对struct_pack类型信息计算其32位MD5并取高31位而得。    
低1位： 数据是否包含元信息字段。0说明不存在元信息字段。1说明存在元信息字段。

![](./images/layout/hash_info.svg)

# 2. 元信息 （可选）

当类型哈希的最后一位为1时，struct_pack会包含元信息字段。元信息由一字节的元信息头和若干字节的元信息组成。

## 2.1 元信息头

元信息头一共一个字节。描述了struct_pack的配置情况。


目前的配置项包括：

| 配置项        | 含义                                                                                     | 所占比特的位置 | 默认情况                                                       | 是否可手动配置 |
| ------------- | ---------------------------------------------------------------------------------------- | -------------- | -------------------------------------------------------------- | -------------- |
| LEN_SIZE      | 容器中的长度字段，其所占的字节数                                                         | 0b0000011      | 由序列化对象内最长的容器的长度决定                             | 否             |
| ADD_TYPE_INFO | 序列化数据是否包含完整类型信息                                                           | 0b0000100      | Debug模式下包含，Release模式下不包含，也可以由用户手动配置     | 是             |
| DATA_SIZE     | 序列化数据是否包含向后兼容字段，如果包含向后兼容字段，则记录序列化结果的长度所占的字节数 | 0b0011000      | 如果待序列化的类型包含compatible字段，则记录总长度，否则不记录 | 否             |



从低位到高位，各设置分别为：

1. `DATA_SIZE`。共两比特。当包含compatible兼容字段时，反序列化时无法确定尾部的兼容字段有多长。为保证能够跳过这些未知字段，需要记录序列化结果的总长度。`01`，`10`，`11`代表数据总长度所占的字节数为：$2$字节，$4$字节，$8$字节。`00`代表不记录数据总长度，说明用户序列化的结构体中不包含compatible字段。
2. `ADD_TYPE_INFO`。 是否包含类型信息，共一比特。$0$代表不存在完整类型信息，$1$代表存在。struct_pack在debug模式下会默认包含元信息，release下不会包含，用户也可自行配置。
3. `LEN_SIZE`大小，共$2$bit。`00`,`01`,`10`,`11`分别对应的长度为：$1$字节，$2$字节，$4$字节，$8$字节。该大小是记录最长的容器的长度所需要的字节数。默认值为`00`，即长度小于256。
4. 剩余$3$bit保留，目前应该保持为`000`，有待后续扩展。

![](./images/layout/meta_info.svg)

## 2.2 数据总长度（可选）

如果记录数据总长度，则接下来的元信息字段是数据的总长度，该字段的长度由`DATA_SIZE`字段决定，并使用原码记录一个无符号整型数字，该数字为struct_pack序列化数据的总长度。

## 2.3 完整类型信息（可选）

如果选择记录完整类型信息，则接下来的字段是完整类型信息。该字段为一个字符串，唯一映射了一个合法的struct_pack类型。以`'\0'`为结尾，记录了被序列化的结构体的完整类型信息。其经过MD5计算得到的$32$位哈希值的高$31$位即为类型哈希字段中的哈希值。

当类型校验通过时，该字段的长度可在编译期确定。当校验失败时，该字段的长度需要根据`'\0'`字符的位置确定。

该字段的编码规则以及它和struct_pack类型树的映射关系请见[struct_pack的类型系统]。

例如，对于类型：
```cpp
struct person {
  int age;
  std::string name;
};
```
其类型信息字段的内容为：

![类型信息](./images/layout/person_type_info.svg)


# 3. 已知数据负载（必选）

该部分负载了用户的序列化数据。

## 3.1 结构体字段

结构体(struct,class,pair,tuple...)，是一系列字段的组合。在struct_pack的内存布局中，结构体本身并不包含额外的信息，所有的成员字段按从低到高的顺序排布就组成了整个结构体的内存布局。

例如, 我们定义一个person类型:
```cpp
struct person {
  int age;
  std::string name;
};
```

在release模式下，序列化person{.age=24,.name="Betty"}对象，默认得到的结果为：

![release模式下person的序列化编码](./images/layout/release_person.svg)

在debug模式下，序列化person{.age=24,.name="Betty"}对象，默认会添加元信息（类型信息）用于避免哈希冲突，因此其结果为：

![debug模式下person的序列化编码](./images/layout/debug_person.svg)

在release模式下，序列化person{.age=24,.name=std::string{256,'A'}}对象，默认得到的结果为：

![](./images/layout/release_person_long_name.svg)


## 3.2 基本类型字段

基本类型字段都是定长的，包含有符号无符号整数，浮点数，字符，枚举和布尔类型。

### 定长无符号整数字段


| 类型名   | 编码长度（字节) | 编码格式 |
| -------- | --------------- | -------- |
| uint8_t  | 1               | 原码     |
| uint16_t | 2               | 原码     |
| uint32_t | 4               | 原码     |
| uint64_t | 8               | 原码     |

含uint8_t,uint16_t,uint32_t,uint64_t类型。编码格式为原码。

### 定长有符号整数字段

| 类型名  | 编码长度（字节) | 编码格式 |
| ------- | --------------- | -------- |
| int8_t  | 1               | 补码     |
| int16_t | 2               | 补码     |
| int32_t | 4               | 补码     |
| int64_t | 8               | 补码     |

含int8_t,int16_t,int32_t,int64_t类型。编码格式为补码。

### 定长浮点数字段

| 类型名 | 编码长度（字节) | 编码格式         |
| ------ | --------------- | ---------------- |
| float  | 4               | IEEE-754(单精度) |
| double | 8               | IEEE-754(双精度) |

含float（32位）和double（64位）。编码为IEEE754。

### 字符字段

含char_8_t,char16_t,char32_t,wchar_t类型。编码格式为原码。

| 类型名   | 编码长度（字节) | 编码格式 |
| -------- | --------------- | -------- |
| char8_t  | 1               | 原码     |
| char16_t | 2               | 原码     |
| char32_t | 4               | 原码     |
| wchar_t  | 由平台决定      | 原码     |

### 枚举字段

枚举字段的内存布局和枚举类型对应的整数类型的布局相同。

### 布尔字段

bool类型，占一个字节。当该字节的所有比特均为0时表示`false`，否则为`true`

## 3.3 数据结构字段

struct_pack支持多种数据结构，不同的数据结构有着不同的内存布局。

### 定长数组字段

包含C-style的数组类型或C++的std::array类型，或类型的自定义数据结构。

该字段的内存布局不包含长度信息（长度信息在编译期已知）。其布局为数组的成员按从低到高的顺序依次排列。

例如，序列化对象包含字段`std::array<int,2>{24,42}`,该字段的编码如下图所示：

![](images/layout/array_layout.svg)

### container字段

包含顺序容器和字符串类型，如`std::vector`,`std::deque`,`std::list`，`std::string`或类似的自定义数据结构。

该字段的内存布局，首先是一个size_type大小的长度字段，该字段为无符号整数，编码类型为原码。接着是若干个container的成员，按下标从小到大排布。其数量由长度字段决定。

例如，序列化对象包含字段`std::string{"Hello"}`，该字段的编码如下图所示：

![](images/layout/string_layout.svg)

### set字段

包含`std::set` 或类似的自定义数据结构。

该字段的内存布局，首先是一个size_type大小的长度字段，该字段为无符号整数，编码类型为原码。接着是若干个set的key，按顺序排布，其数量由长度字段决定。

例如，序列化对象包含字段`std::set<int>{42,24}`，该字段的编码如下图所示：

![](images/layout/set_layout.svg)

### map字段

包含`std::map` 或类似的自定义数据结构。

该字段的内存布局，首先是一个size_type大小的长度字段，该字段为无符号整数，编码类型为原码。接着是若干个map的value_type字段，其数量由长度字段决定。

例如，序列化对象包含字段`std::map<int,std::string>{{42，"Hello"},{24,"Student"}}`，该字段的编码如下图所示：

![](images/layout/map_layout.svg)


### optional字段

包含`std::optional`或类似的自定义数据结构。

该字段的内存布局头部为一个布尔，当其为假时，代表optional为空，编码到此为止。否则，后面接着optional字段的成员类型。

例如，序列化对象包含字段`std::optional<int>{42}`，该字段的编码如下图所示：

![](images/layout/optional_has_value.svg)


例如，序列化对象包含字段`std::optional<int>{std::nullopt}`，该字段的编码如下图所示：

![](images/layout/optional_null.svg)

### variant字段

包含`std::variant`或类似的自定义数据结构。

该字段的内存布局头部为一个字节，存储variant的Index，其为无符号整数，按原码编码，标识variant实际存储的类型是哪一种。后面接着该类型的编码。

例如，序列化对象包含字段`std::variant<int,std::string,double>{3.14}`，该字段的编码如下图所示：

![](images/layout/variant.svg)

### expected字段

包含`std::expected`或类似的自定义数据结构。

该字段的内存布局头部为一个布尔，当布尔为真时，代表expected包含的是期望值，后面接着期望值的编码。否则，代表expected包含的是错误码，后面接着错误码字段的编码。

例如，序列化对象包含字段`std::expected<int,std::errc>{42}`，该字段的编码如下图所示：

![](./images/layout/variant.svg)

例如，序列化对象包含字段`std::expect<int,std::errc>{std::unexpected{std::errc::no_buffer_space}}`，该字段的编码如下图所示：

![](./images/layout/variant_with_error.svg)

### monostate字段

空类型，其编码长度为零。

# 4. 未知字段负载（可选）

该部分数据负载了可以向前或向后兼容的数据，由compatible字段组成，其数量未知。当存在该部分数据时，元信息中一定记录了数据的总长度。

反序列化时，对于未知的compatible字段应该直接忽略并跳过该部分。对于未包含的compatible字段应反序列化为空值。

compatible字段的排布顺序与其在结构体中的定义顺序相同，按定义顺序由低字节向高字节排布。单个compatible字段的编码规则和optional字段的编码规则相同。

例如，对于结构体

struct person {
  int age;
  std::string name;
  struct_pack::compatible<double> salary;
};

则在release模式下序列化person{.age=24,.name="Betty",.salary=2000.0}，其编码格式为：     
![](./images/layout/release_person_with_compatible.svg)









