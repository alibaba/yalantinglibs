set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
find_package(Protobuf)
add_executable(test_serialize
        test_serialize.cpp
        test_compile_time_calculate.cpp
        test_data_struct.cpp
        test_tuplet.cpp
        test_alignas.cpp
        test_pragma_pack.cpp
        test_pragma_pack_and_alignas_mix.cpp
        test_varint.cpp
        main.cpp
        )
add_test(NAME test_serialize COMMAND test_serialize)
target_compile_definitions(test_serialize PRIVATE STRUCT_PACK_ENABLE_UNPORTABLE_TYPE)
target_link_libraries(test_serialize PRIVATE libstruct_pack doctest)
target_compile_definitions(test_serialize PRIVATE
        STRUCT_PACK_ENABLE_UNPORTABLE_TYPE
        )
target_link_libraries(test_serialize PRIVATE
        libstruct_pack
        doctest
        )
if (Protobuf_FOUND)
    message(STATUS "Protobuf_FOUND: ${Protobuf_FOUND}")
    target_compile_definitions(test_serialize PRIVATE HAVE_PROTOBUF)
    target_link_libraries(test_serialize PRIVATE protobuf::libprotobuf)
    protobuf_generate_cpp(TEST_SERIALIZE_PROTO_SRCS
            TEST_SERIALIZE_PROTO_HDRS
            test_pb.proto
            ../benchmark/benchmark.proto
            )
    target_include_directories(test_serialize PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
    target_sources(test_serialize PRIVATE
            ${TEST_SERIALIZE_PROTO_SRCS}
            ${TEST_SERIALIZE_PROTO_HDRS}
            )
    message(STATUS "TEST_SERIALIZE_PROTO_SRCS: ${TEST_SERIALIZE_PROTO_SRCS}")
    message(STATUS "TEST_SERIALIZE_PROTO_HDRS: ${TEST_SERIALIZE_PROTO_HDRS}")
endif ()

add_executable(test_struct_pb_gen
        test_struct_pb_proxy.cpp
        main.cpp
        )
add_test(NAME test_struct_pb_gen COMMAND test_struct_pb_gen)
target_compile_definitions(test_struct_pb_gen PRIVATE
        STRUCT_PACK_ENABLE_UNPORTABLE_TYPE
        )
target_link_libraries(test_struct_pb_gen PRIVATE
        libstruct_pack
        doctest
        protobuf::libprotobuf
        )
target_include_directories(test_struct_pb_gen PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
target_sources(test_struct_pb_gen PRIVATE
        ${TEST_SERIALIZE_PROTO_SRCS}
        ${TEST_SERIALIZE_PROTO_HDRS}
        )
add_subdirectory(conformance)
