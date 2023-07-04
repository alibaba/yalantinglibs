set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/tests)
find_package(Protobuf QUIET)
if (Protobuf_FOUND)
add_executable(test_struct_pb
        test_pb.cpp
        test_pb_oneof.cpp
        test_pb_benchmark_struct.cpp
        test_pb_bad_identifiers.cpp
        )
target_sources(test_struct_pb PRIVATE
        main.cpp
        )
add_test(NAME test_struct_pb COMMAND test_struct_pb)
target_include_directories(test_struct_pb PUBLIC ${yaLanTingLibs_SOURCE_DIR}/src/struct_pack/benchmark)
    target_compile_definitions(test_struct_pb PRIVATE HAVE_PROTOBUF)
    target_link_libraries(test_struct_pb PRIVATE protobuf::libprotobuf)
    # generate .pb.cc .pb.h
    protobuf_generate_cpp(PROTO_SRCS
            PROTO_HDRS
            test_pb.proto
            data_def.proto
            )
    target_include_directories(test_struct_pb PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
    target_sources(test_struct_pb PRIVATE
            ${PROTO_SRCS}
            ${PROTO_HDRS}
            )
    protobuf_generate_struct_pb(STRUCT_PB_PROTO_SRCS
            STRUCT_PB_PROTO_HDRS
            test_pb.proto
            OPTION "generate_eq_op=true,namespace=test_struct_pb"
            )
    target_sources(test_struct_pb PRIVATE
            ${STRUCT_PB_PROTO_SRCS}
            ${STRUCT_PB_PROTO_HDRS}
            )
    protobuf_generate_struct_pb(STRUCT_PB_TEST_BENCHMARK_PROTO_SRCS
            STRUCT_PB_TEST_BENCHMARK_PROTO_HDRS
            data_def.proto
            OPTION "namespace=struct_pb_sample"
            )
    target_sources(test_struct_pb PRIVATE
            ${STRUCT_PB_TEST_BENCHMARK_PROTO_SRCS}
            ${STRUCT_PB_TEST_BENCHMARK_PROTO_HDRS}
            )
    protobuf_generate_struct_pb(STRUCT_PB_PROTO_SRCS2
            STRUCT_PB_PROTO_HDRS2
            test_bad_identifiers.proto
            test_large_enum_value.proto
            )
    target_sources(test_struct_pb PRIVATE
            ${STRUCT_PB_PROTO_SRCS2}
            ${STRUCT_PB_PROTO_HDRS2}
            )
endif ()