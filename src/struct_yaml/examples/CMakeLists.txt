if("${yaLanTingLibs_SOURCE_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
  # If this is a subproject in ylt
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/examples)
else()
  # else find installed yalantinglibs
    cmake_minimum_required(VERSION 3.15)
    project(coro_rpc_examples)
    if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Release")
    endif()
    set(CMAKE_CXX_STANDARD 20)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
    find_package(yalantinglibs REQUIRED)
    link_libraries(yalantinglibs::yalantinglibs)
endif()

add_executable(struct_yaml_example
        main.cpp
        )