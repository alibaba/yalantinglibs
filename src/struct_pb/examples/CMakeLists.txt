set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/examples)
find_package(Protobuf QUIET)
if (Protobuf_FOUND)
    add_executable(struct_pb_example main.cpp)
    target_protos_struct_pb(struct_pb_example PUBLIC demo.proto)

    add_executable(struct_pb_tutorial tutorial.cpp)
    target_protos_struct_pb(struct_pb_tutorial PRIVATE addressbook.proto)
endif()
