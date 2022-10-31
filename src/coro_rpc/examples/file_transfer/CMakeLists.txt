cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

include_directories(${CMAKE_SOURCE_DIR}/thirdparty/asio)
include_directories(${CMAKE_SOURCE_DIR}/thirdparty/async_simple)
include_directories(${CMAKE_SOURCE_DIR}/thirdparty/spdlog)

add_definitions(-DASIO_HAS_STD_INVOKE_RESULT)
add_definitions(-DASYNC_SIMPLE_HAS_NOT_AIO)

add_executable(file_server file_server/rpc_service.cpp file_server/main.cpp)
add_executable(file_client file_client/main.cpp)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_link_options(file_server PRIVATE "-fuse-ld=lld")
    target_link_options(file_client PRIVATE "-fuse-ld=lld")
    message(STATUS "Force add link option: -fuse-ld=lld")
endif ()