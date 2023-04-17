<p align="center">
<h1 align="center">yaLanTingLibs</h1>
<h6 align="center">C++20基础工具库集合，包括struct_pack, struct_json, struct_pb, easylog, coro_rpc, coro_http 和 async_simple </h6>
</p>
<p align="center">
<img alt="license" src="https://img.shields.io/github/license/alibaba/async_simple?style=flat-square">
<img alt="language" src="https://img.shields.io/github/languages/top/alibaba/yalantinglibs?style=flat-square">
<img alt="last commit" src="https://img.shields.io/github/last-commit/alibaba/async_simple?style=flat-square">
</p>

yaLanTingLibs 是一个C++20基础工具库的集合, 现在它包括 struct_pack, struct_json, struct_pb, easylog, coro_rpc, coro_http 和 [async_simple](https://github.com/alibaba/async_simple), 目前我们正在开发并添加更多的新功能。

yaLanTingLibs 的目标: 为C++开发者提供高性能，极度易用的C++20基础工具库, 帮助用户构建高性能的现代C++应用。

| 测试平台 (编译器版本)                            | 状态                                                                                                   |
|------------------------------------------------|----------------------------------------------------------------------------------------------------------|
| Ubuntu 22.04 (clang 14.0.0)                    | ![ubuntu-clang](https://github.com/alibaba/yalantinglibs/actions/workflows/ubuntu_clang.yml/badge.svg?branch=main) |
| Ubuntu 22.04 (gcc 11.2.0)                      | ![ubuntu-gcc](https://github.com/alibaba/yalantinglibs/actions/workflows/ubuntu_gcc.yml/badge.svg?branch=main)   |
| macOS Monterey 12 (AppleClang 14.0.0.14000029) | ![macos-clang](https://github.com/alibaba/yalantinglibs/actions/workflows/mac.yml/badge.svg?branch=main)         |
| Windows Server 2022 (MSVC 19.33.31630.0)       | ![win-msvc](https://github.com/alibaba/yalantinglibs/actions/workflows/windows.yml/badge.svg?branch=main)     |

## coro_rpc

coro是一个高度易用, head-only，基于协程的C++20高性能rpc框架库。在pipeline echo模式下单机每核心qps可达40万+。

你可以在几分钟之内就构建一个高性能的rpc服务端和客户端！

[简介](https://alibaba.github.io/yalantinglibs/zh/guide/coro-rpc-intro.html)  

[API](https://alibaba.github.io/yalantinglibs/cn/html/group__coro__rpc.html)

[Purecpp 演讲稿](https://alibaba.github.io/yalantinglibs/coro_rpc_introduction_purecpp_talk.pdf).

[Purecpp 会议视频](https://live.csdn.net/room/csdnlive1/bKFbKP7T)，从04:55:08开始。

### 快速开始

1. 定义一个普通函数作为rpc函数。

```cpp
// rpc_service.hpp
inline std::string_view echo(std::string_view str) { return str; }
```

2. 注册rpc函数并启动服务器

```cpp
#include "rpc_service.hpp"
#include <coro_rpc/coro_rpc_server.hpp>

int main() {
  coro_rpc_server server(/*thread_num =*/10, /*port =*/9000);
  server.register_handler<echo>(); // register function echo
  server.start(); // start the server & block
}
```

3. rpc客户端连接服务器并调用rpc函数

```cpp
#include "rpc_service.hpp"
#include <coro_rpc/coro_rpc_client.hpp>

Lazy<void> test_client() {
  coro_rpc_client client;
  co_await client.connect("localhost", /*port =*/"9000"); // connect to the server

  auto r = co_await client.call<echo>("hello coro_rpc"); // call remote function echo
  std::cout << r.result.value() << "\n"; //will print "hello coro_rpc"
}

int main() {
  syncAwait(test_client());
}
```
更多示例[请见](https://github.com/alibaba/yalantinglibs/tree/main/src/coro_rpc/examples).

## struct_pack

struct_pack是一个基于编译期反射，易用且高性能的序列化库，head-only。一行代码即可完成序列化/反序列化。性能远超protobuf等传统序列化库。

[简介与设计文档](https://alibaba.github.io/yalantinglibs/zh/guide/struct-pack-intro.html)

[API文档](https://alibaba.github.io/yalantinglibs/cn/html/group__struct__pack.html)

[(Slides) A Faster Serialization Library Based on Compile-time Reflection and C++20](https://alibaba.github.io/yalantinglibs/A%20Faster%20Serialization%20Library%20Based%20on%20Compile-time%20Reflection%20and%20C++%2020.pdf) CppCon2022 的演讲稿。

[(Video)  A Faster Serialization Library Based on Compile-time Reflection and C++20](https://www.youtube.com/watch?v=myhB8ZlwOlE) CppCon2022 的演讲视频。

[(Slides) 基于编译期反射和模板元编程的序列化库：struct_pack简介](https://alibaba.github.io/yalantinglibs/struct_pack_introduce_CN.pdf) Purecpp的演讲稿。

[(Video) 基于编译期反射和模板元编程的序列化库：struct_pack简介](https://live.csdn.net/room/csdnlive1/bKFbKP7T) Purecpp的演讲视频, 从 01:32:20 开始

### 快速开始
```cpp
// 定义一个C++结构体
struct person {
  int64_t id;
  std::string name;
  int age;
  double salary;
};
// 初始化对象
person person1{.id = 1, .name = "hello struct pack", .age = 20, .salary = 1024.42};

// 一行代码序列化
std::vector<char> buffer = struct_pack::serialize(person1);

// 一行代码反序列化
auto person2 = deserialize<person>(buffer);
```
更多示例[请见](https://github.com/alibaba/yalantinglibs/tree/main/src/struct_pack/examples).

## struct_json
基于反射的json库，轻松实现结构体和json之间的映射。

### 快速开始
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

## coro_http

coro_http 是一个 C++20 的协程http(https)客户端, 支持: get/post, websocket, multipart file , chunked 和 ranges 请求。

### get/post
```cpp
#include "coro_http/coro_http_client.h"
using namespace ylt;

async_simple::coro::Lazy<void> get_post(coro_http_client &client) {
  std::string uri = "http://www.example.com";
  auto result = co_await client.async_get(uri);
  std::cout << result.status << "\n";
  
  result = co_await client.async_post(uri, "hello", req_content_type::string);
  std::cout << result.status << "\n";
}

int main() {
  coro_http_client client{};
  async_simple::coro::syncAwait(get_post(client));
}
```

### websocket
```cpp
async_simple::coro::Lazy<void> websocket(coro_http_client &client) {
  client.on_ws_close([](std::string_view reason) {
    std::cout << "web socket close " << reason << std::endl;
  });
  
  client.on_ws_msg([](resp_data data) {
    std::cout << data.resp_body << std::endl;
  });

  // connect to your websocket server.
  bool r = co_await client.async_connect("ws://example.com/ws");
  if (!r) {
    co_return;
  }

  co_await client.async_send_ws("hello websocket");
  co_await client.async_send_ws("test again", /*need_mask = */ false);
  co_await client.async_send_ws_close("ws close reason");
}
```

### 上传/下载
```cpp
async_simple::coro::Lazy<void> upload_files(coro_http_client &client) {
  std::string uri = "http://example.com";
  
  client.add_str_part("hello", "world");
  client.add_str_part("key", "value");
  client.add_file_part("test", "test.jpg");
  auto result = co_await client.async_upload(uri);
  std::cout << result.status << "\n";
  
  result = co_await client.async_upload(uri, "test", "test.jpg");
}

async_simple::coro::Lazy<void> download_files(coro_http_client &client) {
  // chunked download
  auto result = co_await client.async_download("http://example.com/test.jpg",
                                               "myfile.jpg");
  std::cout << result.status << "\n";
  
  // ranges download
  result = co_await client.async_download("http://example.com/test.txt",
                                               "myfile.txt", "1-10,11-16");
  std::cout << result.status << "\n";
}
```

## async_simple

async_simple是一个C++20协程库，提供各种轻量且易用的组件，帮助用户编写异步代码。
请见[async_simple](https://github.com/alibaba/async_simple)

## 编译器要求

确保你的编译器版本不低于:
- clang11++ (libstdc++-8 以上)。
- g++10 或更高版本。
- msvc 14.29 或更高版本。

## 快速开始（从源码构建）

- 克隆仓库

```shell
git clone https://github.com/alibaba/yalantinglibs.git
```

- 构建，测试并安装（linux/macos）

```shell
cd yalantinglibs
mkdir build && cd build
cmake .. 
# 你可以使用这些选项来跳过构建单元测试/benchmark/样例： 
# cmake .. -DBUILD_EXAMPLES=OFF -DBUILD_BENCHMARK=OFF -DBUILD_UNIT_TESTS=OFF
make         # 如果你的机器性能良好，请使用：make -j 并行编译
ctest .      # 执行测试
make install # 将头文件安装到系统的默认include路径下。
```

- 构建与测试（windows）

使用支持cmake工程的软件，如Visual Studio，Clion，Visual Studio Code打开下载好的源代码，根据IDE提示进行构建和测试。

- 开始编程

以下是我们提供的示例代码，你可以在此基础上构建你的工程：

### coro_rpc

```shell
cd yalantinglibs/src/coro_rpc/examples/helloworld
mkdir build && cd build
cmake ..
make
# 关于更多细节, 请见helloworld下的Cmakelist.txt
```

### struct_pack

TODO

## Benchmark

### coro_rpc

选项:

```bash
./benchmark_client # [线程数（默认为硬件线程数）] [每线程客户端数（默认为20）] [pipeline大小(默认为1，当设为1时相当于ping-pong模式)] [主机地址（默认为127.0.0.1）] [端口号（默认为9000）] [测试数据文件夹地址（默认为"./test_data/echo_test"] [测试秒数（默认为30）] [热身秒数（默认为5）]
```

## 编译选项

| 选项              | 描述                                              |默认值    |
| ----------------- | ------------------------------------------------ | ------- |
| CMAKE_BUILD_TYPE  | 构建类型                                          | Release |
| BUILD_WITH_LIBCXX | 是否使用libc++构建                                | OFF     |
| BUILD_EXAMPLES    | 是否构建样例代码                                   | ON      |
| BUILD_BENCHMARK   | 是否构建性能测试代码                               | ON      | 
| BUILD_UNIT_TESTS  | 是否构建单元测试                                   | ON      |
| USE_CONAN         | 是否使用conan包管理器来管理第三方依赖               | OFF     |
| ENABLE_SSL        | 是否启用SSL支持                                    | OFF     |
| ENABLE_IO_URING   | 是否启用io_uring支持                               | OFF     |

## Dependencies

我们依赖`doctest`来进行第三方测试。
所有第三方库都位于`include/thirdparty`文件夹下。

### coro_rpc

- [struct_pack](https://github.com/alibaba/yalantinglibs)
- [easylog](https://github.com/alibaba/yalantinglibs)
- [asio](https://github.com/chriskohlhoff/asio)
- openssl (optional)
### struct_pack

无第三方依赖。

### struct_json

- [iguana](https://github.com/qicosmos/iguana)

### struct_pb

TODO

### easylog

无第三方依赖。

# 如何生成文档

请见[生成网站](https://alibaba.github.io/yalantinglibs/README.html)

# 如何贡献代码
1. 根据issue模板提交一个issue。
2. 在本地修改代码，通过测试并使用 `git-clang-format HEAD^` 格式化代码。
3. 创建一个Pull Request，填写模板中的内容。
4. 提交Pull Request，并选择审核者: (如： qicosmos, poor-circle, PikachuHyA).
5. 通过github的全平台测试，审核者完成审核，代码合入主线。

# 许可证

yaLanTingLibs 基于 Apache License (Version 2.0) 开发。
本产品包含了许多基于其他开源许可证的第三方组件。
你可以查看[NOTICE文件](/NOTICE)来了解更多信息。