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
        test_stream.cpp
        main.cpp
        )
add_test(NAME test_serialize COMMAND test_serialize)
target_compile_definitions(test_serialize PRIVATE STRUCT_PACK_ENABLE_UNPORTABLE_TYPE)
target_link_libraries(test_serialize PRIVATE libstruct_pack doctest)
