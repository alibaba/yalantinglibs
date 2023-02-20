# This is the cmake script that told you how to compile helloworld project of yalantinglibs
# yalantinglib is head-only. So relax, it's easy.
# Here are 3 examples about how to compile it. 
#########################################################################################
# Example 1 (suggested): Build helloworld with cmake target
#
# You must install yalantinglib first:
#
# git clone https://github.com/alibaba/yalantinglibs
# cd yalantinglibs
# mkdir build && cd build
# cmake ..
# make -j
# make install
#
#########################################################################################
cmake_minimum_required(VERSION 3.15)
project(helloworld)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
if("${yaLanTingLibs_SOURCE_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
  # if is the subproject of yalantinglibs
  # do nothing
else()
  # else find installed yalantinglibs
    find_package(yalantinglibs REQUIRED)
    find_package(Threads)
endif()
add_executable(helloworld_server
        server/main.cpp
        server/rpc_service.cpp
        )
target_link_libraries(helloworld_server PRIVATE yalantinglibs::coro_rpc)
add_executable(helloworld_client client/main.cpp)
target_link_libraries(helloworld_client PRIVATE yalantinglibs::coro_rpc)
add_executable(helloworld_concurrency_clients concurrency_clients/main.cpp)
target_link_libraries(helloworld_concurrency_clients PRIVATE yalantinglibs::coro_rpc)

if("${yaLanTingLibs_SOURCE_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
  # if is the subproject of yalantinglibs
  # do nothing
else()
  # else find installed yalantinglibs
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        # When using coro_rpc_client, only remote function declarations are required.
        # Generally speaking, RPC function declaration and definition are divided.
        # However, clang + ld(gold linker) + '-g' will report 'undefined reference to xxx'.
        # We force use lld when compiler is clang.
        # Note: gcc + ld + '-g' or clang + lld + '-g' work well
        target_link_options(helloworld_server PRIVATE "-fuse-ld=lld")
        target_link_options(helloworld_client PRIVATE "-fuse-ld=lld")
        target_link_options(helloworld_concurrency_client PRIVATE "-fuse-ld=lld")
        message(STATUS "Force add link option: -fuse-ld=lld")
    endif()
endif()
#########################################################################################
# Example 2: Build helloworld without cmake find_target
# You need:
#     - replace ${yaLanTingLibs_SOURCE_DIR} with the real path in your computer.
#########################################################################################
# cmake_minimum_required(VERSION 3.15)
# project(helloworld)
# if(NOT CMAKE_BUILD_TYPE)
#     set(CMAKE_BUILD_TYPE "Release")
# endif()
# set(CMAKE_CXX_STANDARD 20)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_INCLUDE_CURRENT_DIR ON)
# add_executable(helloworld_server
#         server/main.cpp
#         server/rpc_service.cpp
#         )
# add_executable(helloworld_client client/main.cpp)
# target_compile_definitions(helloworld_server PRIVATE ASIO_HAS_STD_INVOKE_RESULT)
# target_compile_definitions(helloworld_server PRIVATE ASYNC_SIMPLE_HAS_NOT_AIO)
# target_compile_definitions(helloworld_client PRIVATE ASIO_HAS_STD_INVOKE_RESULT)
# target_compile_definitions(helloworld_client PRIVATE ASYNC_SIMPLE_HAS_NOT_AIO)
# target_include_directories(helloworld_server PRIVATE
#         ${yaLanTingLibs_SOURCE_DIR}/include
#         ${yaLanTingLibs_SOURCE_DIR}/thirdparty/asio
#         ${yaLanTingLibs_SOURCE_DIR}/thirdparty/async_simple
#         # /root/yalantinglibs/thirdparty/iguana
# )
# target_include_directories(helloworld_client PRIVATE
#         ${yaLanTingLibs_SOURCE_DIR}/include
#         ${yaLanTingLibs_SOURCE_DIR}/thirdparty/asio
#         ${yaLanTingLibs_SOURCE_DIR}/thirdparty/async_simple
# )
# target_link_libraries(helloworld_server -lpthread)
# target_link_libraries(helloworld_client -lpthread)
# # workaround
# if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
#     # When using coro_rpc_client, only remote function declarations are required.
#     # Generally speaking, RPC function declaration and definition are divided.
#     # However, clang + ld(gold linker) + '-g' will report 'undefined reference to xxx'.
#     # We force use lld when compiler is clang.
#     # Note: gcc + ld + '-g' or clang + lld + '-g' work well
#     target_link_options(helloworld_server PRIVATE "-fuse-ld=lld")
#     target_link_options(helloworld_client PRIVATE "-fuse-ld=lld")
#     message(STATUS "Force add link option: -fuse-ld=lld")
# endif ()
#########################################################################################
# Example 3: Compile helloworld without cmake script:
# You need: 
#     - Define macro ASIO_HAS_STD_INVOKE_RESULT and ASYNC_SIMPLE_HAS_NOT_AIO
#     - Add the include path of coro_rpc & asio & async_simple
#     - Add link option with pthread if you use g++/clang++
#     - We suggest link by lld when you use clang++
#########################################################################################