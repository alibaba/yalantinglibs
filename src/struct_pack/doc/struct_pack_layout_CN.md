# struct_pack 的编码规则与布局

**默认情况**下，struct_pack的内存布局如下图所示：

![](./images/layout/memory_layout1.svg)

开头4个字节是类型哈希字段，随后是实际数据的负载，该负载长度是可变的(variable length)，由用户传入的序列化对象决定。

默认情况指：**元信息头**的各个字段均为零值（默认值），此时struct_pack可以省略元信息来压缩长度。需要**同时**满足以下条件才属于**默认情况**：
1. struct_pack不包含compatible字段
2. struct_pack不存储完整类型信息
3. struct_pack的容器最大长度小于256。


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

其详细布局如下图所示：


![](./images/layout/meta_info.svg)


目前的配置项包括：

| 配置项        | 含义                                                                                     | 所占比特的位置 | 影响因素                                                   | 用户是否可指定 |
| ------------- | ---------------------------------------------------------------------------------------- | -------------- | ---------------------------------------------------------- | -------------- |
| DATA_SIZE     | 序列化数据是否包含向后兼容字段，如果包含向后兼容字段，则记录序列化结果的长度所占的字节数 | 0b000000**     | 取决于待序列化的类型是否包含compatible字段，               | 不可手动配置   |
| ADD_TYPE_INFO | 序列化数据是否包含完整类型信息                                                           | 0b00000*00     | Debug模式下包含，Release模式下不包含，也可以由用户手动配置 | 允许手动配置   |
| LEN_SIZE      | 容器的长度序列化后所占的字节数                                                           | 0b000**000     | 由序列化对象里最长的容器的长度决定                         | 不可手动配置   |



从低位到高位，各设置分别为：

1. `DATA_SIZE`。共两比特。当包含compatible兼容字段时，反序列化时无法确定尾部的兼容字段有多长。为保证能够跳过这些未知字段，需要记录序列化结果的总长度。`01`，`10`，`11`代表数据总长度所占的字节数为：2字节，4字节，8字节。`00`代表不记录数据总长度，说明用户序列化的结构体中不包含compatible字段。
2. `ADD_TYPE_INFO`。 是否包含类型信息，共一比特。0代表不存在完整类型信息，1代表存在。struct_pack在debug模式下会默认包含元信息，release下不会包含，用户也可自行配置，详见：
3. `LEN_SIZE`大小，共2bit。`00`,`01`,`10`,`11`分别对应的长度为：1字节，2字节，4字节，8字节。该大小是记录最长的容器的长度所需要的字节数。默认值为`00`，即长度小于256。
4. 剩余3bit保留，目前应该保持为`000`，有待后续扩展。




## 2.2 数据总长度（可选）

如果记录数据总长度，则接下来的元信息字段是数据的总长度，该字段的长度由`DATA_SIZE`字段决定，并使用原码记录一个无符号整型数字，该数字为struct_pack序列化数据的总长度。

## 2.3 完整类型信息（可选）

如果选择记录完整类型信息，则接下来的字段是完整类型信息。该字段为一个字符串，唯一映射了一个合法的struct_pack类型。以`'\0'`为结尾，记录了被序列化的结构体的完整类型信息。其经过MD5计算得到的32位哈希值的高31位即为类型哈希字段中的哈希值。

当类型校验通过时，该字段的长度可在编译期确定。当校验失败时，该字段的长度需要根据`'\0'`字符的位置确定。

该字段的编码规则以及它和struct_pack类型树的映射关系请见[struct_pack的类型系统](https://alibaba.github.io/yalantinglibs/zh/guide/struct-pack-type-system.html)。

例如，对于类型：
```cpp
struct person {
  int age;
  std::string name;
};
```
其类型信息字段的内容为：

![类型信息](./images/layout/person_type_info.svg)

注意类型信息的最后一位为`\0`。

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

在release模式下，序列化`person{.age=24,.name="Betty"}`对象，默认得到的结果为：

![release模式下person的序列化编码](./images/layout/release_person.svg)

上图不存在元信息头，因为其为默认值（`0x0`），因此可以直接省略。

在debug模式下，序列化`person{.age=24,.name="Betty"}`对象，默认会添加元信息（类型信息）用于避免哈希冲突，因此其结果为：

![debug模式下person的序列化编码](./images/layout/debug_person.svg)

上图元信息头为`0x04`，代表元信息含类型信息。


在release模式下，序列化`person{.age=24,.name=std::string{256,'A'}}`对象，默认得到的结果为：

![](./images/layout/release_person_long_name.svg)

上图元信息头为`0x08`，代表`size_type`的长度为`2`。


## 3.2 基本类型字段

基本类型字段都是定长的，包含有符号无符号整数，浮点数，字符，枚举和布尔类型。

### 定长无符号整数字段


| 类型名   | 编码长度（字节) | 编码格式 |
| -------- | --------------- | -------- |
| uint8_t  | 1               | 原码     |
| uint16_t | 2               | 原码     |
| uint32_t | 4               | 原码     |
| uint64_t | 8               | 原码     |

含`uint8_t`,`uint16_t`,`uint32_t`,`uint64_t`类型。编码格式为原码。

### 定长有符号整数字段

| 类型名  | 编码长度（字节) | 编码格式 |
| ------- | --------------- | -------- |
| int8_t  | 1               | 补码     |
| int16_t | 2               | 补码     |
| int32_t | 4               | 补码     |
| int64_t | 8               | 补码     |

含`int8_t`,`int16_t`,`int32_t`,`int64_t`类型。编码格式为补码。

### 定长浮点数字段

| 类型名 | 编码长度（字节) | 编码格式         |
| ------ | --------------- | ---------------- |
| float  | 4               | IEEE-754(单精度) |
| double | 8               | IEEE-754(双精度) |

含`float`（32位）和`double`（64位）。编码为IEEE754。


### 变长无符号整数字段


| 类型名                    | 编码长度（字节) | 编码格式   |
| ------------------------- | --------------- | ---------- |
| struct_pack::var_uint32_t | 1-5 (变长)      | varint编码 |
| struct_pack::var_uint64_t | 1-10  (变长)    | varint编码 |

含`struct_pack::var_uint32_t`,`struct_pack::var_uint64_t`类型。编码格式为varint编码。


### 变长有符号整数字段


| 类型名                   | 编码长度（字节) | 编码格式          |
| ------------------------ | --------------- | ----------------- |
| struct_pack::var_int32_t | 1-5   (变长)    | varint+zigzag编码 |
| struct_pack::var_int64_t | 1-10  (变长)    | varint+zigzag编码 |

含`struct_pack::var_uint32_t`,`struct_pack::var_uint64_t`类型。编码格式为varint+zigzag编码。

### 字符字段


| 类型名   | 编码长度（字节) | 编码格式 |
| -------- | --------------- | -------- |
| char8_t  | 1               | 原码     |
| char16_t | 2               | 原码     |
| char32_t | 4               | 原码     |
| wchar_t  | 由平台决定      | 原码     |


含`char_8_t`,`char16_t`,`char32_t`,`wchar_t`类型。编码格式为原码。

### 枚举字段

枚举字段的内存布局和枚举类型对应的整数类型的布局相同。

### 布尔字段

bool类型，占一个字节。当该字节的所有比特均为0时表示`false`，否则为`true`

## 3.3 数据结构字段

struct_pack支持多种数据结构，不同的数据结构有着不同的内存布局。

### 定长数组字段

包含C-style的数组类型或C++的`std::array`类型，或类型的自定义数据结构。

该字段的内存布局不包含长度信息（长度信息在编译期已知）。其布局为数组的成员按从低到高的顺序依次排列。

例如，序列化对象包含字段`std::array<int,2>{24,42}`,该字段的编码如下图所示：

![](images/layout/array_layout.svg)

### `container`字段

包含顺序容器和字符串类型，如`std::vector`,`std::deque`,`std::list`，`std::string`或类似的自定义数据结构。

该字段的内存布局，首先是一个`size_type`大小的长度字段，该字段为无符号整数，编码类型为原码。接着是若干个`container`的成员，按下标从小到大排布。其数量由长度字段决定。

例如，序列化对象包含字段`std::string{"Hello"}`，该字段的编码如下图所示：

![](images/layout/string_layout.svg)

### `set`字段

包含`std::set` 或类似的自定义数据结构。

该字段的内存布局，首先是一个`size_type`大小的长度字段，该字段为无符号整数，编码类型为原码。接着是若干个`set`的`key`，其数量由长度字段决定。

例如，序列化对象包含字段`std::set<int>{42,24}`，该字段的编码如下图所示：

![](images/layout/set_layout.svg)

### `map`字段

包含`std::map<K,V>` 或类似的自定义数据结构。

该字段的内存布局，首先是一个`size_type`大小的长度字段，该字段为无符号整数，编码类型为原码。接着是若干个`map`的`value_type`字段，其数量由长度字段决定。

例如，序列化对象包含字段

```cpp
std::map<int,std::string>{{42, "Hello"},{24,"Student"}}
```

，该字段的编码如下图所示：

![](images/layout/map_layout.svg)


### `optional<T>`字段

包含`std::optional<T>`或类似的自定义数据结构。

该字段的内存布局头部为一个布尔，当其为假时，代表`optional`为空，编码到此为止。否则，后面接着`optional`字段的成员类型。

例如，序列化对象包含字段`std::optional<int>{42}`，该字段的编码如下图所示：

![](images/layout/optional_has_value.svg)


例如，序列化对象包含字段`std::optional<int>{std::nullopt}`，该字段的编码如下图所示：

![](images/layout/optional_null.svg)

### `variant`字段

包含`std::variant`或类似的自定义数据结构。

该字段的内存布局头部为一个字节，存储`variant`的`index`，其为无符号整数，按原码编码，标识`variant`实际存储的类型是哪一种。后面接着该类型的编码。

例如，序列化对象包含字段`std::variant<int,std::string,double>{3.14}`，该字段的编码如下图所示：

![](images/layout/variant.svg)

### `expected<T,E>`字段

包含`std::expected<T,E>`或类似的自定义数据结构。

该字段的内存布局头部为一个布尔，当布尔为真时，代表`expected`包含的是期望值，后面接着期望值的编码。否则，代表`expected`包含的是错误码，后面接着错误码字段的编码。

例如，序列化对象包含字段`std::expected<int,std::errc>{42}`，该字段的编码如下图所示：

![](./images/layout/variant.svg)

例如，序列化对象包含字段`std::expect<int,std::errc>{std::unexpected{std::errc::no_buffer_space}}`，该字段的编码如下图所示：

![](./images/layout/variant_with_error.svg)

### `monostate`字段

空类型，其编码长度为零。

# 4. 未知字段负载（可选）

该部分数据负载了可以向前或向后兼容的数据，由`compatible<T>`字段组成，其数量未知。当存在该部分数据时，元信息中一定记录了数据的总长度。

反序列化时，对于未知的`compatible<T>`字段应该直接忽略并跳过该部分。对于未包含的`compatible<T>`字段应反序列化为空值。

`compatible<T>`字段按以下规则决定排布顺序：
1. 版本号不同的，按版本号从小到大排布，版本号较小的排在前面。
2. 版本相同时，按字段在结构体内的位置排布。先出现在结构体声明中的字段排在前面。

单个`compatible<T>`字段的编码规则和`optional<T>`字段的编码规则相同。

例如，对于以下结构体：

```cpp
struct person {
  int age;
  std::string name;
  struct_pack::compatible<double> salary;
};
```

则在release模式下序列化对象`person{.age=24,.name="Betty",.salary=2000.0}`，其编码结果为：     
![](./images/layout/release_person_with_compatible.svg)

上图中元信息头为`0x01`，代表结构体包含`compatible<T>`字段，且结构体的总长度小于65536。
