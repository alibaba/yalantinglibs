set(CMAKE_CXX_STANDARD 17)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/tests)
add_executable(easylog_test
        test_easylog.cpp
        main.cpp
        )
add_test(NAME easylog_test COMMAND easylog_test)
target_compile_definitions(easylog_test PRIVATE STRUCT_PACK_ENABLE_UNPORTABLE_TYPE)
if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        target_link_libraries(easylog_test PRIVATE -lstdc++fs)
endif()

