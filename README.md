<p align="center">
<h1 align="center">yaLanTingLibs</h1>
<h6 align="center">A Collection of C++20 libraries, include struct_pack, struct_json, coro_rpc and async_simple </h6>
</p>
<p align="center">
<img alt="license" src="https://img.shields.io/github/license/alibaba/async_simple?style=flat-square">
<img alt="language" src="https://img.shields.io/github/languages/top/alibaba/yalantinglibs?style=flat-square">
<img alt="last commit" src="https://img.shields.io/github/last-commit/alibaba/async_simple?style=flat-square">
</p>

yaLanTingLibs is a collection of C++20 libraries, now it contains struct_pack, struct_json, coro_rpc and [async_simple](https://github.com/alibaba/async_simple), more and more cool libraries will be added into yaLanTingLibs(such as http.) in the future.

The target of yaLanTingLibs: provide very easy and high performance C++20 libraries for C++ developers, it can help to quickly build high performance applications.

| OS (Compiler Version)                          | Status                                                                                                   |
|------------------------------------------------|----------------------------------------------------------------------------------------------------------|
| Ubuntu 22.04 (clang 14.0.0)                    | ![win](https://github.com/alibaba/yalantinglibs/actions/workflows/linux_clang.yml/badge.svg?branch=main) |
| Ubuntu 22.04 (gcc 11.2.0)                      | ![win](https://github.com/alibaba/yalantinglibs/actions/workflows/linux_gcc.yml/badge.svg?branch=main)   |
| macOS Monterey 12 (AppleClang 14.0.0.14000029) | ![win](https://github.com/alibaba/yalantinglibs/actions/workflows/mac.yml/badge.svg?branch=main)         |
| Windows Server 2022 (MSVC 19.33.31630.0)       | ![win](https://github.com/alibaba/yalantinglibs/actions/workflows/windows.yml/badge.svg?branch=main)     |

## coro_rpc

Very easy-to-use, coroutine-based, high performance rpc framework with C++20, more than 2000w qps in echo scene. coro_rpc is a header only library.

You can finish a rpc server and rpc client in 5 minutes!

[English Introduction](https://alibaba.github.io/yalantinglibs/guide/coro-rpc-intro.html) | [中文简介](https://alibaba.github.io/yalantinglibs/zh/guide/coro-rpc-intro.html)  

English API(TODO) | [中文API](https://alibaba.github.io/yalantinglibs/cn/html/group__coro__rpc.html)

[Talk](https://alibaba.github.io/yalantinglibs/coro_rpc_introduction_purecpp_talk.pdf) of coro_rpc on purecpp conference.

[Video](https://live.csdn.net/room/csdnlive1/bKFbKP7T) on purecpp conference, start from 04:55:08 of the video record.

### quick example

1.define a rpc function as a local normal function.

```cpp
// rpc_service.hpp
inline std::string echo(std::string str) { return str; }
```

2.register rpc function and start a server

```cpp
#include "rpc_service.hpp"
#include <coro_rpc/coro_rpc_server.hpp>

int main() {
  coro_rpc_server server(/*thread_num =*/10, /*port =*/9000);
  server.register_handler<echo>();
  server.start();
}
```

3.rpc client call rpc service

```cpp
#include "rpc_service.hpp"
#include <coro_rpc/coro_rpc_client.hpp>

Lazy<void> test_client() {
  coro_rpc_client client;
  co_await client.connect("localhost", /*port =*/"9000");

  auto r = co_await client.call<echo>("hello coro_rpc"); //传参数调用rpc函数
  std::cout << r.result.value() << "\n"; //will print "hello coro_rpc"
}

int main() {
  syncAwait(test_client());
}
```
More examples [here](https://github.com/alibaba/yalantinglibs/tree/main/src/coro_rpc/examples).

## struct_pack

Based on compile-time reflection, very easy to use, high performance serialization library, struct_pack is a header only library, it is used by coro_rpc now.

Only one line code to finish serialization and deserialization, 10-50x faster than protobuf.

[English Introduction](https://alibaba.github.io/yalantinglibs/guide/struct-pack-intro.html) | [中文简介](https://alibaba.github.io/yalantinglibs/zh/guide/struct-pack-intro.html)

English API(TODO) | [中文API](https://alibaba.github.io/yalantinglibs/cn/html/group__struct__pack.html)

[(Slides) A Faster Serialization Library Based on Compile-time Reflection and C++ 20](https://alibaba.github.io/yalantinglibs/A%20Faster%20Serialization%20Library%20Based%20on%20Compile-time%20Reflection%20and%20C++%2020.pdf) of struct_pack on CppCon2022

[Slides](https://alibaba.github.io/yalantinglibs/struct_pack_introduce_CN.pdf) of struct_pack on purecpp conference.

[(Video) A Faster Serialization Library Based on Compile-time Reflection and C++ 20](https://www.youtube.com/watch?v=myhB8ZlwOlE)  on cppcon2022

[Video](https://live.csdn.net/room/csdnlive1/bKFbKP7T) on purecpp conference, start from 01:32:20 of the video record.

### quick example
```cpp
struct person {
  int64_t id;
  std::string name;
  int age;
  double salary;
};

person person1{.id = 1, .name = "hello struct pack", .age = 20, .salary = 1024.42};

// one line code serialize
std::vector<char> buffer = struct_pack::serialize(person1);

// one line code deserialization
auto person2 = deserialize<person>(buffer);
```
See more examples [here](https://github.com/alibaba/yalantinglibs/tree/main/src/struct_pack/examples).

## struct_json
reflection-based json lib, very easy to do struct to json and json to struct.

### quick example
```cpp
#include "struct_json/json_reader.h"
#include "struct_json/json_writer.h"

struct person {
  std::string name;
  int age;
};
REFLECTION(person, name, age);

int main() {
  person p{.name = "tom", .age = 20};
  std::string str;
  struct_json::to_json(p, str); // {"name":"tom","age":20}

  person p1;
  struct_json::from_json(p1, str);
}
```
## async_simple

A C++ 20 coroutine library offering simple, light-weight and easy-to-use components to write asynchronous codes.
See [async_simple](https://github.com/alibaba/async_simple)

## Quick Start of coro_rpc

- clone repo

```shell
git clone https://github.com/alibaba/yalantinglibs.git
```

- build with cmake

```shell
mkdir build && cd build
cmake ..
make -j
```

- run tests

```shell
cd tests
ctest .
```

## Benchmark

options:

```bash
./benchmark_client [threads] [client_pre_thread] [pipeline_size] [host] [port] [test_data_path] [test_time] [warm_up_time]
```

## Build Options

| option            | description                                      | default |
| ----------------- | ------------------------------------------------ | ------- |
| CMAKE_BUILD_TYPE  | build type                                       | Release |
| BUILD_WITH_LIBCXX | Build with libc++                                | OFF     |
| USE_CONAN         | Use conan package manager to handle dependencies | OFF     |
| ENABLE_SSL        | Enable ssl support                               | OFF     |
| ENABLE_IO_URING   | Enable io_uring support                          | OFF     |

## Dependencies

- [doctest](https://github.com/doctest/doctest)
- [asio](https://github.com/chriskohlhoff/asio)
- openssl (optional)
- [async_simple](https://github.com/alibaba/async_simple)
- [iguana](https://github.com/qicosmos/iguana)

Currently, asio and frozen are put in thirdparty folder.
doctest is put in tests folder.

# How to generate document

For English document, run

```shell
doxygen Doxyfile
```
All files generated in `docs/en`.

For Chinese document, run

```shell
doxygen Doxyfile_cn
```
All files generated in `docs/cn`.

# How to Contribute
1. Create an issue in the issue template.
2. Run tests and `git-clang-format HEAD^` locally for the change.
3. Create a PR, fill in the PR template.
4. Choose one or more reviewers from contributors: (e.g., qicosmos, poor-circle, PikachuHyA).
5. Get approved and merged.

# Discussion group

DingTalk group

<center>
<img src="./src/coro_rpc/doc/images/yalantinglibs_ding_talk_group.png" alt="dingtalk" width="200" height="200" align="bottom" />
</center>

# License

yaLanTingLibs is distributed under the Apache License (Version 2.0)
This product contains various third-party components under other open-source licenses.
See the NOTICE file for more information.
