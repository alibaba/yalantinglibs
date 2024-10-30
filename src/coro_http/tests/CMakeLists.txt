set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/tests)
if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Release")
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
find_package(Threads REQUIRED)
link_libraries(Threads::Threads)

include_directories(include)
include_directories(include/ylt/thirdparty)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines")
        #-ftree-slp-vectorize with coroutine cause link error. disable it util gcc fix.
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-tree-slp-vectorize")
endif()

add_executable(coro_http_test
        test_coro_http_server.cpp
        test_cinatra.cpp
        test_cinatra_websocket.cpp
        test_http_parse.cpp
        main.cpp
        )
add_test(NAME coro_http_test COMMAND coro_http_test)
# target_compile_definitions(easylog_test PRIVATE STRUCT_PACK_ENABLE_UNPORTABLE_TYPE)
if (YLT_ENABLE_SSL)
    message(STATUS "Use SSL")
    find_package(OpenSSL REQUIRED)
    add_definitions(-DCINATRA_ENABLE_SSL)
    target_link_libraries(coro_http_test  OpenSSL::SSL OpenSSL::Crypto)
endif ()
