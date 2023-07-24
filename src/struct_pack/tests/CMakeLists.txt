set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/tests)
add_executable(struct_pack_test
        test_serialize.cpp
        test_compile_time_calculate.cpp
        test_data_struct.cpp
        test_data_struct2.cpp
        test_tuplet.cpp
        test_alignas.cpp
        test_pragma_pack.cpp
        test_pragma_pack_and_alignas_mix.cpp
        test_varint.cpp
        test_stream.cpp
        test_compatible.cpp
        test_non_aggregated_type.cpp
        main.cpp
        )
add_test(NAME struct_pack_test COMMAND struct_pack_test)
target_compile_definitions(struct_pack_test PRIVATE STRUCT_PACK_ENABLE_UNPORTABLE_TYPE)
