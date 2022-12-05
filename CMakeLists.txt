cmake_minimum_required(VERSION 3.15)
project(yaLanTingLibs
        VERSION 1.0.0
        DESCRIPTION "yaLanTingLibs"
        HOMEPAGE_URL "https://github.com/alibaba/yalantinglibs"
        LANGUAGES CXX
        )

include_directories(include)

include(cmake/utils.cmake)
include(cmake/build.cmake)
include(cmake/platform.cmake)
include(cmake/dependency.cmake)
include(cmake/develop.cmake)
include(cmake/install.cmake)
include(cmake/module.cmake)