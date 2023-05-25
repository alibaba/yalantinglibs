option(ENABLE_LIBAIO "enable libaio" OFF)
if(ENABLE_LIBAIO)
    message(STATUS "enable libaio")
    add_definitions(-DASIO_HAS_FILE)
    add_definitions(-DASIO_DISABLE_IO_URING)
    add_definitions(-DASIO_HAS_LIB_AIO)
endif()

option(ENABLE_FILE_IO_URING "enable io_uring" OFF)
if(ENABLE_FILE_IO_URING)
    message(STATUS "enable io_uring")
    add_definitions(-DASIO_HAS_FILE)
    add_definitions(-DASIO_HAS_IO_URING)
endif()

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

## manual import
include_directories(${yaLanTingLibs_SOURCE_DIR}/thirdparty/asio)
include_directories(${yaLanTingLibs_SOURCE_DIR}/thirdparty/async_simple)

add_executable(coro_file_example
        main.cpp
        )

if(ENABLE_LIBAIO)
if (UNIX)
    target_link_libraries(coro_file_example -lpthread aio)
endif()
endif()

if(ENABLE_FILE_IO_URING)
if (UNIX)
    target_link_libraries(coro_file_example -luring)
endif()
endif()
