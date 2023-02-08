find_package(Protobuf)
if (Protobuf_FOUND)
    file(GLOB SRCS "*.cpp" "*.h")
    add_executable(protoc-gen-struct_pb ${SRCS})
    target_link_libraries(protoc-gen-struct_pb PUBLIC protobuf::libprotoc protobuf::libprotobuf)
    if (NOT TARGET yalantinglibs::protoc-gen-struct_pb)
        add_executable(yalantinglibs::protoc-gen-struct_pb ALIAS protoc-gen-struct_pb)
    endif ()
    ylt_install(protoc-gen-struct_pb)
else ()
    message(WARNING "to build struct_pb protoc plugin, you must install libprotoc first!!!\n"
            "see https://alibaba.github.io/yalantinglibs/guide/struct-pb-generating-your-struct.html"
            )
endif ()
