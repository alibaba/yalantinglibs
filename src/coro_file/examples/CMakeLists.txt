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

add_executable(coro_file_example
        main.cpp
        )

if(ENABLE_FILE_IO_URING)
if (UNIX)
    target_link_libraries(coro_file_example -luring)
endif()
endif()
