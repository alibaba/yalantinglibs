set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/tests)
add_executable(metric_test
        test_metric.cpp
        parallel_test.cpp
        )
target_compile_options(metric_test PRIVATE -fsanitize=address)
target_link_options(metric_test PRIVATE -fsanitize=address)
add_test(NAME metric_test COMMAND metric_test)
