option(ENABLE_LIBAIO "enable libaio" OFF)
if(ENABLE_LIBAIO)
    message(STATUS "enable libaio")
    add_definitions(-DASIO_HAS_FILE)
    add_definitions(-DASIO_DISABLE_IO_URING)
    add_definitions(-DASIO_HAS_LIB_AIO)
endif()

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
