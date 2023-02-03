add_executable(benchmark_coro_server coro_server.cpp)
target_link_libraries(benchmark_coro_server PRIVATE libcoro_rpc)
target_include_directories(benchmark_coro_server PRIVATE . ..)

