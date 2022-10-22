add_executable(benchmark_coro_server coro_server.cpp)
target_link_libraries(benchmark_coro_server PRIVATE libcoro_rpc)
target_include_directories(benchmark_coro_server PRIVATE . ..)

add_executable(benchmark_async_server async_server.cpp)
target_link_libraries(benchmark_async_server PRIVATE libcoro_rpc)
target_include_directories(benchmark_async_server PRIVATE . ..)

