if(NOT BUILD_SP_BENCHMARK)
    message(WARNING "additional BUILD_SP_BENCHMARK=ON needed for struct_pack benchmark")
    return()
endif()

find_package(Protobuf REQUIRED)
find_program(PROTOC_CXX_FOUND protoc DOC "Protobuf Compiler (protoc)")
if(NOT PROTOC_CXX_FOUND)
    message(FATAL_ERROR "install protoc before struct_pack benchmark")
endif()

execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/benchmark/)
execute_process(COMMAND protoc benchmark.proto --cpp_out=. -I ${CMAKE_CURRENT_SOURCE_DIR}
                WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/benchmark)
add_executable(struct_pack_benchmark benchmark.cpp no_op.cpp ${CMAKE_BINARY_DIR}/benchmark/benchmark.pb.cc)

target_include_directories(struct_pack_benchmark PUBLIC ${CMAKE_BINARY_DIR}/benchmark/)
target_include_directories(struct_pack_benchmark PRIVATE
        ${CMAKE_SOURCE_DIR}/thirdparty/frozen
        )
target_link_libraries(struct_pack_benchmark ${Protobuf_LIBRARIES})
target_compile_definitions(struct_pack_benchmark PRIVATE MSGPACK_NO_BOOST STRUCT_PACK_OPTIMIZE)
