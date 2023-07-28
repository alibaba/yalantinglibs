
if("${yaLanTingLibs_SOURCE_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
  # if is the subproject of yalantinglibs
  # do nothing
else()
  # else find installed yalantinglibs
    cmake_minimum_required(VERSION 3.15)
    project(file_transfer)
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Release")
    endif()
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
    find_package(Threads REQUIRED)
    link_libraries(Threads::Threads)
  # if you have install ylt  
    find_package(yalantinglibs REQUIRED)
    link_libraries(yalantinglibs::yalantinglibs)
  # else 
  # include_directories(include)
  # include_directories(include/ylt/thirdparty)
  
  # When using coro_rpc_client to send request, only remote function declarations are required.
  # In the examples, RPC function declaration and definition are divided.
  # However, clang + ld(gold linker) + '-g' will report 'undefined reference to xxx'.
  # So you can use lld when compiler is clang by this code:
  # if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  # add_link_options(-fuse-ld=lld)    
  # endif()

  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines")
    #-ftree-slp-vectorize with coroutine cause link error. disable it util gcc fix.
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-tree-slp-vectorize")
  endif()
endif()

add_executable(coro_rpc_example_channel channel.cpp)
add_executable(coro_rpc_example_client_pool client_pool.cpp)
add_executable(coro_rpc_example_client_pools client_pools.cpp)
add_executable(coro_rpc_example_client client.cpp)
add_executable(coro_rpc_example_concurrent_clients concurrent_clients.cpp)
add_executable(coro_rpc_example_server server.cpp rpc_service.cpp)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_SYSTEM_NAME MATCHES "Windows") # mingw-w64
  target_link_libraries(coro_rpc_example_channel wsock32 ws2_32)
  target_link_libraries(coro_rpc_example_client_pool wsock32 ws2_32)
  target_link_libraries(coro_rpc_example_client_pools wsock32 ws2_32)
  target_link_libraries(coro_rpc_example_client wsock32 ws2_32)
  target_link_libraries(coro_rpc_example_concurrent_clients wsock32 ws2_32)
  target_link_libraries(coro_rpc_example_server wsock32 ws2_32)
endif()

