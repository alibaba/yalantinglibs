set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/tests)
if(NOT CMAKE_BUILD_TYPE)
        set(CMAKE_BUILD_TYPE "Release")
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
find_package(Threads REQUIRED)
find_package(ibverbs REQUIRED)
link_libraries(ibverbs Threads::Threads)

include_directories(include)
include_directories(include/ylt/thirdparty)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines")
        #-ftree-slp-vectorize with coroutine cause link error. disable it util gcc fix.
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-tree-slp-vectorize")
endif()

add_executable(coro_rdma_test
        test_rdma.cpp
        )


add_test(NAME coro_rdma_test COMMAND coro_rdma_test)
