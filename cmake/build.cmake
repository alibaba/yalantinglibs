# Compile Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
message(STATUS "CXX Standard: ${CMAKE_CXX_STANDARD}")

# Build Type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# libc++ or libstdc++&clang
option(BUILD_WITH_LIBCXX "Build with libc++" OFF)
if(BUILD_WITH_LIBCXX AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    message(STATUS "Build with libc++")
else()
    message(STATUS "Build with libstdc++")
endif()

# ccache
option(USE_CCACHE "use ccache to faster compile when develop" OFF)
if (USE_CCACHE)
    find_program(CCACHE_FOUND ccache)
    if (CCACHE_FOUND AND USE_CCACHE)
        set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
        #set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache) # ccache for link is useless
        message(STATUS "ccache found")
    else ()
        message(WARNING "ccache not found :you'd better use ccache to faster compile if you are developer")
    endif ()
endif ()
