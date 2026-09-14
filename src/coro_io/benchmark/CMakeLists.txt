set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/benchmark)

if(YLT_HAS_OWN_RING)
    add_executable(coro_file_bench coro_file_bench.cpp)
    add_executable(coro_file_bench_uring coro_file_bench.cpp)
    target_compile_definitions(coro_file_bench_uring PRIVATE ASIO_DISABLE_EPOLL)
endif()
