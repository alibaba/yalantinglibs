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
        test_derived.cpp
        test_cross_platform.cpp
        test_disable_meta_info.cpp
        main.cpp
        )
add_test(NAME struct_pack_test COMMAND struct_pack_test)
include (TestBigEndian)
TEST_BIG_ENDIAN(IS_BIG_ENDIAN)
if(NOT IS_BIG_ENDIAN)
target_compile_definitions(struct_pack_test PRIVATE STRUCT_PACK_ENABLE_UNPORTABLE_TYPE  TEST_IN_LITTLE_ENDIAN)
else()
target_compile_definitions(struct_pack_test PRIVATE STRUCT_PACK_ENABLE_UNPORTABLE_TYPE)
endif()
add_custom_command(
        TARGET struct_pack_test PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/binary_data
        ${CMAKE_BINARY_DIR}/output/tests/binary_data)
add_custom_command(
        TARGET struct_pack_test PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/binary_data
        ${CMAKE_BINARY_DIR}/src/struct_pack/tests/binary_data)
