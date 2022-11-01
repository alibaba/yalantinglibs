add_executable(benchmark_client client.cpp)
target_link_libraries(benchmark_client PRIVATE libcoro_rpc)
target_include_directories(benchmark_client PRIVATE . ..)

add_executable(benchmark_data_gen data_gen.cpp)
target_link_libraries(benchmark_data_gen PRIVATE libcoro_rpc Threads::Threads)
target_include_directories(benchmark_data_gen PRIVATE . ..)

message(STATUS "GENERATE_BENCHMARK_DATA")
target_compile_definitions(benchmark_data_gen PRIVATE GENERATE_BENCHMARK_DATA)
add_custom_command(
        TARGET benchmark_data_gen POST_BUILD
        COMMAND ./benchmark_data_gen 
        COMMENT "generate benchmake data"
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/benchmark
        VERBATIM
)
