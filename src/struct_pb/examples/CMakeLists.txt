find_package(Protobuf)
if (Protobuf_FOUND)
    add_executable(struct_pb_example main.cpp)
    #protobuf_generate_cpp(PROTO_SRCS0 PROTO_HDRS0 demo.proto)
    protobuf_generate_struct_pb(PROTO_SRCS PROTO_HDRS demo.proto)
    target_sources(struct_pb_example PRIVATE
            #        ${PROTO_SRCS0} ${PROTO_HDRS0}
            ${PROTO_SRCS} ${PROTO_HDRS}
            )
    target_link_libraries(struct_pb_example PRIVATE libstruct_pb)

    add_executable(struct_pb_tutorial tutorial.cpp)
    protobuf_generate_struct_pb(PROTO_SRCS2 PROTO_HDRS2 addressbook.proto)
    target_sources(struct_pb_tutorial PRIVATE ${PROTO_SRCS2} ${PROTO_HDRS2})
    target_include_directories(struct_pb_tutorial PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
    target_link_libraries(struct_pb_tutorial PRIVATE libstruct_pb)
endif()
