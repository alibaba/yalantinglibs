add_executable(coro_http_example
        main.cpp
        )
target_link_libraries(coro_http_example PRIVATE coro_http)

## manual import coro_http
include_directories(${yaLanTingLibs_SOURCE_DIR}/thirdparty/asio)
include_directories(${yaLanTingLibs_SOURCE_DIR}/thirdparty/cinatra)
include_directories(${yaLanTingLibs_SOURCE_DIR}/thirdparty/async_simple)
