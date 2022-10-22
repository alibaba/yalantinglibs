# struct_pack简介

struct_pack是一个以零成本抽象，高度易用为特色序列化库。通常情况下只需一行代码即可完成复杂结构体的序列化/反序列化。用户无需定义任何DSL，宏或模板代码，struct_pack可通过编译期反射自动支持对C++结构体的序列化。其综合性能比protobuf，msgpack大幅提升(详细可以看benchmark部分)。

# 基本用法

以一个简单的对象为例展示struc_pack的基本用法。

```c++
struct person {
  int64_t id;
  std::string name;
  int age;
  double salary;
};
```

## 序列化

```c++
// 初始化一个person对象
person person1{.id = 1, .name = "hello struct pack", .age = 20, .salary = 1024.42};

// 1行代码序列化
std::vector<char> result = serialize(person1);
```

## 反序列化

```c++
person person2;
// 1行代码反序列化
auto ec = deserialize_to<person>(person2, buffer.data(), buffer.size());
assert(ec == std::errc{});
assert(p1 == p2);
```

## 部分反序列化

有时候只想反序列化对象的某个特定的字段而不是全部，这时候就可以用部分反序列化功能了，这样可以避免全部反序列化，大幅提升效率。

```c++
// 只反序列化person的第2个字段
auto [err, name] = get_field<person, 1>(buffer.data(), buffer.size());

assert(name == "hello struct pack");
```

## 支持序列化所有的STL容器、自定义容器和optional

含各种容器的对象序列化

```c++
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
auto [err, nested1] = deserialize(buffer.data(), buffer.size());
assert(nested==nested1);
```

自定义容器的序列化

```c++
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

# benchmark

## 测试方法

待序列化的对象已经预先初始化，存储序列化结果的内存已经预先分配。对每个测试用例。我们运行一百万次序列化/反序列化，对结果取平均值。

## 测试对象

1. 含有整形、浮点型和字符串类型person对象

```c++
struct person {
  int64_t id;
  std::string name;
  int age;
  double salary;
};
```

2. 含有十几个字段包括嵌套对象的复杂对象monster

```c++
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

3. 含有4个int32的rect对象

```c++
struct rect {
  int32_t x;
  int32_t y;
  int32_t width;
  int32_t height;
};
```

## 测试环境

Compiler: Alibaba Clang 13

CPU: (Intel(R) Xeon(R) Platinum 8163 CPU @ 2.50GHz)

## 测试结果

![](./images/struct_pack_bench.png)

# 向前/向后兼容性

当对象增加新的字段时，怎么保证兼容新旧对象的解析呢？当用户需要添加字段时，只需要在**新对象末尾**
增加新的 `struct_pack::compatible<T>` 字段即可。<br />以person对象为例：

```c++
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

struct_pack保证这两个类可以通过序列化和反序列化实现安全的相互转换，从而实现了向前/向后的兼容性。

# 为什么struct_pack更快？

1. 精简的类型信息，高效的类型校验。MD5计算在编译期完成，运行时只需要比较32bit的hash值是否相同即可。
2. struct_pack是一个模板库，鼓励编译器积极的内联函数。
4. 0成本抽象，不会为用不到的特性付出运行时代价。
5. struct_pack的内存布局更接近于C++结构体原始的内存布局，减少了序列化反序列化的工作量。
6. 编译期类型计算允许struct_pack根据不同的类型生成不同的代码。因此我们可以根据不同的类型的特点做优化。例如对于连续容器可以直接memcpy，对于string_view反序列化时可以采用零拷贝优化。

# 附录

## 测试代码

请见 [benchmark.cpp](../benchmark/benchmark.cpp)
