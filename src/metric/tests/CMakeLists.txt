set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/tests)
add_executable(metric_test
        test_metric.cpp
        )
add_test(NAME metric_test COMMAND metric_test)
