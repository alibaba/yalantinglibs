set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
find_package(Protobuf)
add_executable(test_struct_pb
        test_pb.cpp
        test_pb_oneof.cpp
        )
target_sources(test_struct_pb PRIVATE
        main.cpp
        test_pb.struct_pb.h
        test_pb.struct_pb.cc
        )
target_link_libraries(test_struct_pb PRIVATE libstruct_pb doctest)
if (Protobuf_FOUND)
    message(STATUS "Protobuf_FOUND: ${Protobuf_FOUND}")
    target_compile_definitions(test_struct_pb PRIVATE HAVE_PROTOBUF)
    target_link_libraries(test_struct_pb PRIVATE protobuf::libprotobuf)
    protobuf_generate_cpp(PROTO_SRCS
            PROTO_HDRS
            test_pb.proto
            ../benchmark/benchmark.proto
            )
    target_include_directories(test_struct_pb PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
    target_sources(test_struct_pb PRIVATE
            ${PROTO_SRCS}
            ${PROTO_HDRS}
            test_pb_benchmark_struct.cpp
            ../benchmark/benchmark.struct_pb.cc
            ../benchmark/benchmark.struct_pb.h
            )
    message(STATUS "PROTO_SRCS: ${PROTO_SRCS}")
    message(STATUS "PROTO_HDRS: ${PROTO_HDRS}")
endif ()