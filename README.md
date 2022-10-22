# yaLanTingLibs

yaLanTingLibs is a collection of C++20 libraries, it contains coroutine library(async_simple), compile-time reflection based serialization(struct_pack), easy to use rpc(coro_rpc), orm, http, modules etc.

The target of yaLanTingLibs: provide very easy and high performance C++20 libraries for C++ developers, it can help to quickly build high performance applications.

## coro_rpc

Very easy-to-use, coroutine-based, high performance rpc framework with C++20. coro_rpc is a header only library.

## struct_pack

Based on compile-time reflection, very easy to use, high performance serialization library, struct_pack is a header only library, it is used by coro_rpc now.

## Quick Start Of coro_rpc

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

# License

yaLanTingLibs is distributed under the Apache License (Version 2.0)
This product contains various third-party components under other open source licenses.
See the NOTICE file for more information.
