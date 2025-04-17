set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/benchmark)

#set(SIMD_BENCHMARK_COMPILE_FLAGS  -mavx2 -mbmi -mlzcnt)

find_package(benchmark REQUIRED)
if(benchmark_FOUND)
    message(STATUS "Google Benchmark found - enabling benchmarks")
    
    add_executable(simd_util_benchmark main.cpp)
    target_link_libraries(simd_util_benchmark PRIVATE benchmark::benchmark)
    add_test(NAME run_benchmark COMMAND simd_util_benchmark)
else()
    message(WARNING "Google Benchmark NOT found - skipping benchmarks")
endif()
