cmake_minimum_required(VERSION 3.15)
project(yaLanTingLibs
        VERSION 0.2.0
        DESCRIPTION "yaLanTingLibs"
        HOMEPAGE_URL "https://github.com/alibaba/yalantinglibs"
        LANGUAGES CXX
        )

# load pack finder
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/Find/)

# add include path
include_directories(include)
include_directories(include/ylt/thirdparty)
include_directories(src/include)

find_package(Threads REQUIRED)
link_libraries(Threads::Threads)

include(cmake/utils.cmake)
include(cmake/struct_pb.cmake)
include(cmake/build.cmake)
include(cmake/develop.cmake)
include(cmake/install.cmake)
# add project config, such as enable_ssl.
include(cmake/config.cmake)
# add project's source such as unit test, example & benchmark
include(cmake/subdir.cmake)


