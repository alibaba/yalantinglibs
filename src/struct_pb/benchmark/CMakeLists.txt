add_executable(struct_pb_benchmark main.cpp)
find_package(Protobuf)
if (Protobuf_FOUND)
    message(STATUS "Protobuf_FOUND: ${Protobuf_FOUND}")
    target_link_libraries(struct_pb_benchmark PRIVATE protobuf::libprotobuf)
    protobuf_generate_cpp(STRUCT_PB_BENCHMARK_PROTO_SRCS
            STRUCT_PB_BENCHMARK_PROTO_HDRS
            benchmark.proto
            )
    target_include_directories(struct_pb_benchmark PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
    target_sources(struct_pb_benchmark PRIVATE
            ${STRUCT_PB_BENCHMARK_PROTO_SRCS}
            ${STRUCT_PB_BENCHMARK_PROTO_HDRS}
            )
    target_compile_definitions(struct_pb_benchmark PRIVATE HAVE_PROTOBUF)
    protobuf_generate_struct_pb(STRUCT_PB_BENCHMARK_PROTO_SRCS2
            STRUCT_PB_BENCHMARK_PROTO_HDRS2
            benchmark.proto
            OPTION "namespace=struct_pb_sample"
            )
    target_sources(struct_pb_benchmark PRIVATE
            ${STRUCT_PB_BENCHMARK_PROTO_SRCS2}
            ${STRUCT_PB_BENCHMARK_PROTO_HDRS2}
            )
else()
    target_sources(struct_pb_benchmark PRIVATE
            benchmark.struct_pb.cc
            benchmark.struct_pb.h
            )
endif()