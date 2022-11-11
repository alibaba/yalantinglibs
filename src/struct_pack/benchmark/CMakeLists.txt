option(MSGPACK_USE_BOOST OFF)
find_package(Protobuf REQUIRED)
find_package(msgpack REQUIRED)
add_executable(struct_pack_benchmark benchmark.cpp no_op.cpp)
target_link_libraries(struct_pack_benchmark PRIVATE
        msgpackc-cxx
        protobuf::libprotobuf
        )
target_compile_definitions(struct_pack_benchmark PRIVATE MSGPACK_NO_BOOST)
protobuf_generate_cpp(STRUCT_PACK_BENCHMARK_PROTO_SRCS STRUCT_PACK_BENCHMARK_PROTO_HDRS benchmark.proto)
target_include_directories(struct_pack_benchmark PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_sources(struct_pack_benchmark PRIVATE
        ${STRUCT_PACK_BENCHMARK_PROTO_SRCS}
        ${STRUCT_PACK_BENCHMARK_PROTO_HDRS}
        )
message(STATUS "STRUCT_PACK_BENCHMARK_PROTO_SRCS: ${STRUCT_PACK_BENCHMARK_PROTO_SRCS}")
message(STATUS "STRUCT_PACK_BENCHMARK_PROTO_HDRS: ${STRUCT_PACK_BENCHMARK_PROTO_HDRS}")
