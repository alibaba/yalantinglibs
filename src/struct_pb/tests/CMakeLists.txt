find_package(Protobuf QUIET)
if("${yaLanTingLibs_SOURCE_DIR}" STREQUAL "${CMAKE_SOURCE_DIR}")
  # If this is a subproject in ylt
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/tests)
else()
  # else find installed yalantinglibs
    cmake_minimum_required(VERSION 3.15)
    project(struct_pb_test)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
  # if you have install ylt  
    find_package(yalantinglibs REQUIRED)
    link_libraries(yalantinglibs::yalantinglibs)
  # else 
  # include_directories(include)
  # include_directories(include/ylt/thirdparty)
endif()

set(TEST_PROTO main.cpp)

if (Protobuf_FOUND)
  add_definitions(-DSTRUCT_PB_WITH_PROTO)
  message(STATUS "Found Protobuf: ${Protobuf_VERSION}")
  include_directories(${Protobuf_INCLUDE_DIRS})
  include_directories(${CMAKE_CURRENT_BINARY_DIR})
  set(PROTO_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/)
  file(GLOB PROTO_FILES ${PROTO_SRC_DIR}/*.proto)

  protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})
  # message(STATUS "Proto source files: ${PROTO_SRCS}")
  # message(STATUS "Proto header files: ${PROTO_HDRS}")
  add_executable(struct_pb_test ${PROTO_SRCS} ${TEST_PROTO})
  target_link_libraries(struct_pb_test PRIVATE protobuf::libprotobuf)
  add_test(NAME struct_pb_test COMMAND struct_pb_test)
else()
  add_executable(struct_pb_test ${TEST_PROTO})
  add_test(NAME struct_pb_test COMMAND struct_pb_test)
endif()