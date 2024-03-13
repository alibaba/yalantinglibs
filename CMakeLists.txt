cmake_minimum_required(VERSION 3.15)
project(yaLanTingLibs
        VERSION 0.3.0
        DESCRIPTION "yaLanTingLibs"
        HOMEPAGE_URL "https://github.com/alibaba/yalantinglibs"
        LANGUAGES CXX
        )

# load pack finder
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/Find/)

find_package(Threads REQUIRED)
link_libraries(Threads::Threads)

include(cmake/install.cmake)

if(CMAKE_PROJECT_NAME STREQUAL "yaLanTingLibs") # if ylt is top-level project
        # add include path
        include_directories(include)
        include_directories(include/ylt/thirdparty)
        include_directories(src/include)

        include(cmake/utils.cmake)
        include(cmake/struct_pb.cmake)
        include(cmake/build.cmake)
        include(cmake/develop.cmake)
        # add project config, such as enable_ssl.
        include(cmake/config.cmake)
        # add project's source such as unit test, example & benchmark
        include(cmake/subdir.cmake)
else ()
        # add project config, such as enable_ssl.
        include(cmake/config.cmake)
endif()

set(VERSION "0.3.0")
set(NUM_VERSION 300)
set(GIT_HASH "")
get_git_hash(GIT_HASH)
set(GIT_BRANCH "")
get_git_branch(GIT_BRANCH)
message("Git hash is ${GIT_HASH}, branch is ${GIT_BRANCH}")

configure_file(
        ${PROJECT_SOURCE_DIR}/include/ylt/version.hpp.in
        ${PROJECT_SOURCE_DIR}/include/ylt/version.hpp
)
