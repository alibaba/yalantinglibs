<p align="center">
<h1 align="center">yaLanTingLibs</h1>
<h6 align="center">A Collection of C++20 libraries, include async_simple, coro_rpc and struct_pack </h6>
</p>
<p align="center">
<img alt="license" src="https://img.shields.io/github/license/alibaba/async_simple?style=flat-square">
<img alt="language" src="https://img.shields.io/github/languages/top/alibaba/yalantinglibs?style=flat-square">
<img alt="last commit" src="https://img.shields.io/github/last-commit/alibaba/async_simple?style=flat-square">
</p>

yaLanTingLibs is a collection of C++20 libraries, now it contains [async_simple](https://github.com/alibaba/async_simple), struct_pack and coro_rpc.

The target of yaLanTingLibs: provide very easy and high performance C++20 libraries for C++ developers, it can help to quickly build high performance applications.

## async_simple

A library offering simple, light-weight and easy-to-use components to write asynchronous codes.
See [async_simple](https://github.com/alibaba/async_simple)

## coro_rpc

Very easy-to-use, coroutine-based, high performance rpc framework with C++20. coro_rpc is a header only library.

English Introduction(TODO) | [中文简介](./src/coro_rpc/doc/coro_rpc_introduction_cn.md)  

English API(TODO) | [中文API](./src/coro_rpc/doc/coro_rpc_doc.hpp)

## struct_pack

Based on compile-time reflection, very easy to use, high performance serialization library, struct_pack is a header only library, it is used by coro_rpc now.

English Introduction(TODO) | [中文简介](./src//struct_pack/doc/Introduction_CN.md)

English API(TODO) | [中文API](./src/struct_pack/doc/struct_pack_doc.hpp)

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

## Dependencies

- [doctest](https://github.com/doctest/doctest)
- [spdlog](https://github.com/gabime/spdlog)
- [asio](https://github.com/chriskohlhoff/asio)
- openssl (optional)
- [async_simple](https://github.com/alibaba/async_simple)

Currently, asio and spdlog are put in thirdparty folder.
doctest is put in tests folder.

# How to Contribute
1. Create an issue in the issue template.
2. Run tests and `git-clang-format HEAD^` locally for the change.
3. Create a PR, fill in the PR template.
4. Choose one or more reviewers from contributors: (e.g., qicosmos, poor-circle, PikachuHyA).
5. Get approved and merged.

# License

yaLanTingLibs is distributed under the Apache License (Version 2.0)
This product contains various third-party components under other open source licenses.
See the NOTICE file for more information.
