set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
find_package(Protobuf)
add_executable(test_struct_pb
        test_pb.cpp
        test_pb_oneof.cpp
        test_pb_benchmark_struct.cpp
        )
target_sources(test_struct_pb PRIVATE
        main.cpp
        )
target_link_libraries(test_struct_pb PRIVATE libstruct_pb doctest)
add_test(NAME test_struct_pb COMMAND test_struct_pb)
if (Protobuf_FOUND)
    message(STATUS "Protobuf_FOUND: ${Protobuf_FOUND}")
    target_compile_definitions(test_struct_pb PRIVATE HAVE_PROTOBUF)
    target_link_libraries(test_struct_pb PRIVATE protobuf::libprotobuf)
    protobuf_generate_cpp(PROTO_SRCS
            PROTO_HDRS
            test_pb.proto
            benchmark.proto
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
            benchmark.proto
            OPTION "namespace=struct_pb_sample"
            )
    target_sources(test_struct_pb PRIVATE
            ${STRUCT_PB_TEST_BENCHMARK_PROTO_SRCS}
            ${STRUCT_PB_TEST_BENCHMARK_PROTO_HDRS}
            )
else()
    target_sources(test_struct_pb PRIVATE
            test_pb.struct_pb.cc
            test_pb.struct_pb.h
            ${yaLanTingLibs_SOURCE_DIR}/src/struct_pack/benchmark/benchmark.struct_pb.cc
            ${yaLanTingLibs_SOURCE_DIR}/src/struct_pack/benchmark/benchmark.struct_pb.h
            )
endif ()