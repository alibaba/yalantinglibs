cmake_minimum_required(VERSION 3.15)
project(OwnRingConsumer LANGUAGES CXX)

get_filename_component(YLT_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../../.." ABSOLUTE)
add_subdirectory("${YLT_SOURCE_DIR}" yalantinglibs-build)

add_executable(own_ring_consumer main.cpp probe.cpp)
target_link_libraries(own_ring_consumer PRIVATE yalantinglibs::yalantinglibs)
target_compile_definitions(own_ring_consumer PRIVATE YLT_EXPECT_OWN_RING=$<BOOL:${YLT_ENABLE_OWN_RING}>)
enable_testing()
add_test(NAME own_ring_consumer COMMAND own_ring_consumer)
