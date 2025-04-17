set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/benchmark)

#set(SIMD_BENCHMARK_COMPILE_FLAGS  -mavx2 -mbmi -mlzcnt)


include(FetchContent)
FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.10.x)

FetchContent_Declare(
    googlebenchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.8.0
)

FetchContent_MakeAvailable(
        googletest
        googlebenchmark)
    
add_executable(simd_util_benchmark main.cpp)
target_link_libraries(simd_util_benchmark PRIVATE benchmark::benchmark)
add_test(NAME run_benchmark COMMAND simd_util_benchmark)