set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/tests/struct_pb)
find_package(Protobuf QUIET)
if (NOT Protobuf_FOUND)
    message(INFO "to build struct_pb conformance test, you must install libprotoc first")
    return()
endif ()
set(CMAKE_INCLUDE_CURRENT_DIR ON)
add_executable(conformance_test_runner
        binary_json_conformance_suite.cc
        binary_json_conformance_suite.h
        conformance_test_runner.cc
        conformance_test_main.cc
        conformance_test.cc
        conformance_test.h
        )
target_sources(conformance_test_runner PRIVATE
        conformance/conformance.proto
        google/protobuf/test_messages_proto2.proto
        google/protobuf/test_messages_proto3.proto
        )
protobuf_generate(TARGET conformance_test_runner)
target_include_directories(conformance_test_runner PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(conformance_test_runner PRIVATE protobuf::libprotobuf)
add_executable(conformance_cpp conformance_cpp.cc)
target_link_libraries(conformance_cpp PRIVATE protobuf::libprotobuf)
target_sources(conformance_cpp PRIVATE
        conformance/conformance.proto
        google/protobuf/test_messages_proto2.proto
        google/protobuf/test_messages_proto3.proto
        )
protobuf_generate(TARGET conformance_cpp)
target_include_directories(conformance_cpp PRIVATE ${CMAKE_CURRENT_BINARY_DIR})

add_test(NAME conformance_cpp_test
        COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/conformance_test_runner
        --failure_list ${CMAKE_CURRENT_SOURCE_DIR}/failure_list_cpp.txt
        --output_dir ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/conformance_cpp
        DEPENDS conformance_test_runner conformance_cpp)

add_executable(conformance_struct_pb
        conformance_struct_pb.cc
        )
protobuf_generate_struct_pb(SRCS HDRS
        conformance/conformance.proto
        google/protobuf/test_messages_proto2.proto
        google/protobuf/test_messages_proto3.proto
        google/protobuf/any.proto
        google/protobuf/descriptor.proto
        google/protobuf/duration.proto
        google/protobuf/field_mask.proto
        google/protobuf/struct.proto
        google/protobuf/timestamp.proto
        google/protobuf/type.proto
        google/protobuf/wrappers.proto
        google/protobuf/source_context.proto
        TARGET conformance_struct_pb
        )
target_sources(conformance_struct_pb PRIVATE ${SRCS} ${HDRS})
target_include_directories(conformance_struct_pb PRIVATE ${CMAKE_CURRENT_BINARY_DIR})


add_test(NAME conformance_struct_pb_test
        COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/conformance_test_runner
        --failure_list ${CMAKE_CURRENT_SOURCE_DIR}/failure_list_cpp.txt
        --output_dir ${CMAKE_CURRENT_BINARY_DIR}
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/conformance_struct_pb
        DEPENDS conformance_test_runner conformance_struct_pb)
