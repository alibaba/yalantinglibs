find_package(Protobuf)
add_executable(struct_pack_benchmark benchmark.cpp no_op.cpp)
if (Protobuf_FOUND)
    message(STATUS "Protobuf_FOUND: ${Protobuf_FOUND}")
    protobuf_generate_cpp(STRUCT_PACK_BENCHMARK_PROTO_SRCS
            STRUCT_PACK_BENCHMARK_PROTO_HDRS
            data_def.proto
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
            data_def.proto
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

target_compile_definitions(struct_pack_benchmark PRIVATE MSGPACK_NO_BOOST)
target_compile_definitions(struct_pack_benchmark PRIVATE STRUCT_PACK_OPTIMIZE)

find_package(Flatbuffers)
if (Flatbuffers_FOUND)
    message(STATUS "Flatbuffers_FOUND: ${Flatbuffers_FOUND}")
    add_custom_command(
            TARGET struct_pack_benchmark PRE_BUILD
            COMMAND flatc ./data_def.fbs --cpp
            COMMENT "generate flatbuffer"
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            VERBATIM
    )
    target_compile_definitions(struct_pack_benchmark PRIVATE HAVE_FLATBUFFER)
endif()
