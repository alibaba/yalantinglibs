option(MSGPACK_USE_BOOST OFF)
find_package(Protobuf REQUIRED)
execute_process(
        COMMAND protoc benchmark.proto --cpp_out=. -I ${CMAKE_CURRENT_SOURCE_DIR}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/benchmark
)
add_executable(struct_pack_benchmark benchmark.cpp no_op.cpp ${CMAKE_BINARY_DIR}/benchmark/benchmark.pb.cc)
target_include_directories(struct_pack_benchmark PUBLIC ${CMAKE_BINARY_DIR}/benchmark/)
target_link_libraries(struct_pack_benchmark ${Protobuf_LIBRARIES})
target_compile_definitions(struct_pack_benchmark PRIVATE MSGPACK_NO_BOOST)