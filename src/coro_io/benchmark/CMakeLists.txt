set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/benchmark)

if(YLT_HAS_OWN_RING)
    add_executable(coro_file_bench coro_file_bench.cpp)
endif()
