set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/tests)
add_executable(metric_test
        test_metric.cpp
        parallel_test.cpp
        )
check_asan(HAS_ASAN)
if (has_asan) 
        target_compile_options(metric_test PRIVATE -fsanitize=address)
        target_link_options(metric_test PRIVATE -fsanitize=address)
endif()
add_test(NAME metric_test COMMAND metric_test)
