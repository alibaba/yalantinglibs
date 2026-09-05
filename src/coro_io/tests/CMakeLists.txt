set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/tests)
add_executable(coro_io_test
        test_io_context_pool.cpp
        test_corofile.cpp
        test_shared_file_handle.cpp
        test_load_balancer.cpp
        test_client_pool.cpp
        test_rate_limiter.cpp
        test_coro_channel.cpp
        test_cancel.cpp
        main.cpp
        )
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_SYSTEM_NAME MATCHES "Windows") # mingw-w64
    target_link_libraries(coro_io_test PRIVATE ws2_32 mswsock)
endif()
add_test(NAME coro_io_test COMMAND coro_io_test)

if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_executable(coro_file_error_test
            test_file_error_paths.cpp
            main.cpp
            )
    if (CMAKE_SIZEOF_VOID_P EQUAL 8)
        target_link_options(coro_file_error_test PRIVATE
                "-Wl,--wrap=_Znwm"
                "-Wl,--wrap=_Znam"
                )
    elseif (CMAKE_SIZEOF_VOID_P EQUAL 4)
        target_link_options(coro_file_error_test PRIVATE
                "-Wl,--wrap=_Znwj"
                "-Wl,--wrap=_Znaj"
                )
    endif()
    add_test(NAME coro_file_error_test COMMAND coro_file_error_test)
endif()

add_subdirectory(ibverbs)
add_subdirectory(networkdirect)
