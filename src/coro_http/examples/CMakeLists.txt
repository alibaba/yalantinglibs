
if("${yaLanTingLibs_SOURCE_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/examples/coro_http)
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

  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines")
    #-ftree-slp-vectorize with coroutine cause link error. disable it util gcc fix.
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-tree-slp-vectorize")
  endif()
endif()

add_executable(coro_http_example example.cpp)
add_executable(coro_http_load_balancer load_balancer.cpp)
add_executable(coro_chat_room chat_room.cpp)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_SYSTEM_NAME MATCHES "Windows") # mingw-w64
    target_link_libraries(coro_http_example wsock32 ws2_32)
    target_link_libraries(coro_http_load_balancer wsock32 ws2_32)
    target_link_libraries(coro_chat_room wsock32 ws2_32)
endif()

option(ENABLE_pybind11 "Enable pybind11 " OFF)
if(ENABLE_pybind11)
  cmake_minimum_required(VERSION 3.14)
  project(MyPybind11Project)

  include(FetchContent)

  FetchContent_Declare(
    pybind11
    GIT_REPOSITORY https://github.com/pybind/pybind11.git
    GIT_TAG v2.13.6
  )

  FetchContent_MakeAvailable(pybind11)

  include_directories(${CMAKE_CURRENT_BINARY_DIR}/_deps/pybind11-src/include)

  pybind11_add_module(py_example py_example/py_example.cpp)
  set_target_properties(py_example PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/py_example
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/py_example
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/py_example
  )
endif()