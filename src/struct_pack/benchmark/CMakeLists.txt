find_package(Protobuf)
add_executable(struct_pack_benchmark benchmark.cpp no_op.cpp)
if (Protobuf_FOUND)
    protobuf_generate_cpp(STRUCT_PACK_BENCHMARK_PROTO_SRCS
            STRUCT_PACK_BENCHMARK_PROTO_HDRS
            benchmark.proto
            )
    target_include_directories(struct_pack_benchmark PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
    target_sources(struct_pack_benchmark PRIVATE
            ${STRUCT_PACK_BENCHMARK_PROTO_SRCS}
            ${STRUCT_PACK_BENCHMARK_PROTO_HDRS}
            )
    target_link_libraries(struct_pack_benchmark PRIVATE protobuf::libprotobuf)

    target_compile_definitions(struct_pack_benchmark PRIVATE HAVE_PROTOBUF)
    protobuf_generate_struct_pb(STRUCT_PACK_BENCHMARK_PROTO_SRCS2
            STRUCT_PACK_BENCHMARK_PROTO_HDRS2
            benchmark.proto
            OPTION "namespace=struct_pb_sample"
            )
    target_sources(struct_pack_benchmark PRIVATE
            ${STRUCT_PACK_BENCHMARK_PROTO_SRCS2}
            ${STRUCT_PACK_BENCHMARK_PROTO_HDRS2}
            )

    target_compile_definitions(struct_pack_benchmark PRIVATE HAVE_PROTOBUF)
else ()
    target_sources(struct_pack_benchmark PRIVATE
            generated_code/benchmark.struct_pb.cc
            generated_code/benchmark.struct_pb.h
            )
    target_include_directories(struct_pack_benchmark PRIVATE generated_code)
endif ()

# TODO
find_package(Msgpack)
if (Msgpack_FOUND)
    message(STATUS "Msgpack_FOUND: ${Msgpack_FOUND}")
    target_link_libraries(struct_pack_benchmark PRIVATE msgpackc-cxx)
    target_compile_definitions(struct_pack_benchmark PRIVATE HAVE_MSGPACK)
    target_compile_definitions(struct_pack_benchmark PRIVATE MSGPACK_NO_BOOST)
endif()
target_compile_definitions(struct_pack_benchmark PRIVATE STRUCT_PACK_OPTIMIZE)
