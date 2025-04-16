set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/tests/simd_util)

add_executable(util_simd_string_split_test test_simd_string_split.cpp main.cpp)

add_test(NAME util_simd_string_split_test COMMAND util_simd_string_split_test)