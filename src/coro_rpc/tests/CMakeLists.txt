set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
set(TEST_SRCS
        test_async_rpc_server.cpp
        test_coro_rpc_server.cpp
        test_coro_rpc_client.cpp
        test_remote.cpp
        test_register_handler.cpp
        test_router.cpp
        test_connection.cpp
        test_function_name.cpp
        test_variadic.cpp
        )
set(TEST_COMMON
        rpc_service.cpp
        main.cpp
        )

add_executable(test_rpc
        ${TEST_SRCS}
        ${TEST_COMMON}
        )
target_link_libraries(test_rpc PRIVATE libcoro_rpc doctest)
target_compile_definitions(test_rpc PRIVATE UNIT_TEST_INJECT)
target_compile_definitions(test_rpc PRIVATE STRUCT_PACK_ENABLE_UNPORTABLE_TYPE)
add_custom_command(
        TARGET test_rpc POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/openssl_files
        ${CMAKE_BINARY_DIR}/openssl_files)
foreach (_test_file ${TEST_SRCS})
    get_filename_component(_test_name ${_test_file} NAME_WE)
    add_executable(${_test_name}
            ${_test_file}
            ${TEST_COMMON}
            )
    target_link_libraries(${_test_name}
            libcoro_rpc
            doctest
            )
if  (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_definitions(${_test_name} PRIVATE ASIO_DISABLE_IOCP)
endif()
    target_compile_definitions(${_test_name} PRIVATE UNIT_TEST_INJECT)
    add_test(${_test_name} ${_test_name})
    set_tests_properties(${_test_name} PROPERTIES
            TIMEOUT 300
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests
            )
endforeach ()

set(TEST_REGISTER_DUPLICATION_SRCS
        test_register_duplication_1.cpp
        test_register_duplication_2.cpp
        test_register_duplication_3.cpp
        )
foreach (_test_file ${TEST_REGISTER_DUPLICATION_SRCS})
    get_filename_component(_test_name ${_test_file} NAME_WE)
    add_executable(${_test_name}
            ${_test_file}
            rpc_service.cpp
            )
    target_link_libraries(${_test_name}
            libcoro_rpc
            doctest
            )
    add_test(${_test_name} ${_test_name})
    set_tests_properties(${_test_name} PROPERTIES
            WILL_FAIL ON
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tests
            )
endforeach ()
add_executable(test_util test_util.cpp)
target_link_libraries(test_util PRIVATE libcoro_rpc)
