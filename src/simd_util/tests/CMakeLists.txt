set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/tests/simd_util)

#set(SIMD_TEST_COMPILE_FLAGS  -mavx2 -mbmi -mlzcnt)

add_executable(util_simd_string_split_test test_simd_string_split.cpp main.cpp)
#target_compile_options(util_simd_string_split_test PRIVATE ${SIMD_TEST_COMPILE_FLAGS})

add_test(NAME util_simd_string_split_test COMMAND util_simd_string_split_test)