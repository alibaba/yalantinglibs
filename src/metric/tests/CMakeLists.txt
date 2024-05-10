set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/tests)
add_executable(metric_test
        test_metric.cpp
        )
add_test(NAME metric_test COMMAND metric_test)
target_compile_definitions(metric_test PRIVATE STRUCT_PACK_ENABLE_UNPORTABLE_TYPE)
if (NOT ${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        target_link_libraries(metric_test PRIVATE -lstdc++fs)
endif()

