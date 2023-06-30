set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/benchmark)

add_executable(coro_rpc_benchmark_server server.cpp)

add_executable(coro_rpc_benchmark_client client.cpp)

if (GENERATE_BENCHMARK_DATA)
    add_executable(coro_rpc_benchmark_data_gen data_gen.cpp)
    target_compile_definitions(coro_rpc_benchmark_data_gen PRIVATE GENERATE_BENCHMARK_DATA)
    add_custom_command(
            TARGET coro_rpc_benchmark_data_gen POST_BUILD
            COMMAND ./coro_rpc_benchmark_data_gen
            COMMENT "generate benchmark data"
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/output/benchmark
            VERBATIM
    )
endif ()
