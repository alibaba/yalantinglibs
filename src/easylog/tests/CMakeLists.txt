set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)

add_executable(test_easylog
        test_easylog.cpp
        main.cpp
        )
add_test(NAME test_easylog COMMAND test_easylog)
target_compile_definitions(test_easylog PRIVATE STRUCT_PACK_ENABLE_UNPORTABLE_TYPE)
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        target_link_libraries(test_easylog PRIVATE yalantinglibs::easylog doctest)
else()
        target_link_libraries(test_easylog PRIVATE yalantinglibs::easylog doctest -lstdc++fs)
endif()

