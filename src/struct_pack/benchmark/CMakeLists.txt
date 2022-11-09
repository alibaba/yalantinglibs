option(MSGPACK_USE_BOOST OFF)
find_package(Protobuf)
find_package(msgpack)
add_executable(struct_pack_benchmark benchmark.cpp no_op.cpp)
target_link_libraries(struct_pack_benchmark PRIVATE libstruct_pack)
target_compile_definitions(struct_pack_benchmark PRIVATE STRUCT_PACK_OPTIMIZE)
if (Protobuf_FOUND)
    message(STATUS "Protobuf_FOUND: ${Protobuf_FOUND}")
    target_link_libraries(struct_pack_benchmark PRIVATE protobuf::libprotobuf)
    target_compile_definitions(struct_pack_benchmark PRIVATE HAVE_PROTOBUF)
    protobuf_generate_cpp(STRUCT_PACK_BENCHMARK_PROTO_SRCS STRUCT_PACK_BENCHMARK_PROTO_HDRS benchmark.proto)
    target_include_directories(struct_pack_benchmark PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
    target_sources(struct_pack_benchmark PRIVATE
            ${STRUCT_PACK_BENCHMARK_PROTO_SRCS}
            ${STRUCT_PACK_BENCHMARK_PROTO_HDRS}
            )
    message(STATUS "STRUCT_PACK_BENCHMARK_PROTO_SRCS: ${STRUCT_PACK_BENCHMARK_PROTO_SRCS}")
    message(STATUS "STRUCT_PACK_BENCHMARK_PROTO_HDRS: ${STRUCT_PACK_BENCHMARK_PROTO_HDRS}")
endif ()
if (msgpack_FOUND)
    message(STATUS "msgpack_FOUND: ${msgpack_FOUND}")
    target_link_libraries(struct_pack_benchmark PRIVATE msgpackc-cxx)
    target_compile_definitions(struct_pack_benchmark PRIVATE MSGPACK_NO_BOOST)
    target_compile_definitions(struct_pack_benchmark PRIVATE HAVE_MSGPACK)
endif ()
if (EXISTS ${CMAKE_SOURCE_DIR}/thirdparty/protopuf/include)
    message(STATUS "struct_pack benchmark HAVE_PROTOPUF")
    target_include_directories(struct_pack_benchmark PRIVATE
            ${CMAKE_SOURCE_DIR}/thirdparty/protopuf/include
            )
    target_compile_definitions(struct_pack_benchmark PRIVATE HAVE_PROTOPUF)
endif ()

if (EXISTS ${CMAKE_SOURCE_DIR}/thirdparty/protozero/include)
    message(STATUS "struct_pack benchmark HAVE_PROTOZERO")
    target_include_directories(struct_pack_benchmark PRIVATE
            ${CMAKE_SOURCE_DIR}/thirdparty/protozero/include
            )
    target_compile_definitions(struct_pack_benchmark PRIVATE HAVE_PROTOZERO)
endif ()
