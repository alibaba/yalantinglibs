set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/tests)
add_executable(coro_io_test
        test_corofile.cpp
        test_channel.cpp
        test_client_pool.cpp
        main.cpp
        )
add_test(NAME coro_io_test COMMAND coro_io_test)



