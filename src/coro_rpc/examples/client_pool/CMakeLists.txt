cmake_minimum_required(VERSION 3.15)
project(client_pool)
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
add_executable(rpc_client_pool_example main.cpp)
target_link_libraries(rpc_client_pool_example PRIVATE yalantinglibs::coro_rpc)

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
        target_link_options(nested_rpc_call_server PRIVATE "-fuse-ld=lld")
        target_link_options(nested_rpc_call_client PRIVATE "-fuse-ld=lld")
        message(STATUS "Force add link option: -fuse-ld=lld")
    endif()
endif()