find_package(glog QUIET)
find_package(spdlog QUIET)

if (UNIX)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()
# include_directories(spdlog/include)
add_executable(easylog_benchmark
        main.cpp
        )
target_link_libraries(easylog_benchmark PRIVATE yalantinglibs::easylog)
add_test(NAME easylog_benchmark COMMAND easylog_benchmark)

if (glog_FOUND)
  message(STATUS "glog_FOUND: ${glog_FOUND}")
  target_compile_definitions(easylog_benchmark PRIVATE HAVE_GLOG)
  target_link_libraries(easylog_benchmark PRIVATE glog::glog)
endif ()

if (spdlog_FOUND)
  message(STATUS "spdlog_FOUND: ${spdlog_FOUND}")
  target_compile_definitions(easylog_benchmark PRIVATE HAVE_SPDLOG)
  target_link_libraries(easylog_benchmark PRIVATE spdlog::spdlog_header_only)
endif ()
