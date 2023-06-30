cmake_minimum_required(VERSION 3.15)
project(file_transfer)
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

find_package(Msgpack)
if (Msgpack_FOUND)

add_executable(coro_rpc_example_use_rest_rpc_protocol
  server/main.cpp
  server/rpc_service.cpp
)

if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  target_link_libraries(coro_rpc_example_use_rest_rpc_protocol PRIVATE -lstdc++fs)
endif()

target_compile_definitions(coro_rpc_example_use_rest_rpc_protocol PRIVATE HAVE_MSGPACK)
target_compile_definitions(coro_rpc_example_use_rest_rpc_protocol PRIVATE MSGPACK_NO_BOOST)

else()
  message(WARNING "Could Not Found Msgpack, disable coro_rpc_example_use_rest_rpc_protocol")
endif()
