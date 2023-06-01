set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)
option(ENABLE_FILE_IO_URING "enable io_uring" OFF)
if(ENABLE_FILE_IO_URING)
    message(STATUS "enable io_uring")
    add_definitions(-DENABLE_FILE_IO_URING)
    add_definitions(-DASIO_HAS_FILE)
    add_definitions(-DASIO_HAS_IO_URING)
endif()

if (UNIX)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

## manual import
include_directories(${yaLanTingLibs_SOURCE_DIR}/thirdparty/asio)
include_directories(${yaLanTingLibs_SOURCE_DIR}/thirdparty/async_simple)


add_executable(test_corofile
        test_corofile.cpp
        main.cpp
        )

if(ENABLE_FILE_IO_URING)
if (UNIX)
        target_link_libraries(test_corofile PRIVATE yalantinglibs::easylog doctest uring)
endif()
else()
        target_link_libraries(test_corofile PRIVATE yalantinglibs::easylog doctest)
endif()


add_test(NAME test_corofile COMMAND test_corofile)



