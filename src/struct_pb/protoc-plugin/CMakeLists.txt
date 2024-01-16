set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/bin)
find_package(Protobuf QUIET)
if (Protobuf_FOUND)
    file(GLOB SRCS "*.cpp" "*.h")
    add_executable(protoc-gen-struct_pb ${SRCS})
    target_link_libraries(protoc-gen-struct_pb PUBLIC protobuf::libprotoc protobuf::libprotobuf)
    install(
        TARGETS protoc-gen-struct_pb
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
    )
else ()
    message(INFO "struct_pb is skipped. To build struct_pb protoc plugin, you must install libprotoc first\n"

            "see https://alibaba.github.io/yalantinglibs/en/struct_pb/struct_pb_generating_your_struct.html"
            )
endif ()
