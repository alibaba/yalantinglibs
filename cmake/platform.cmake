# --------------------- Gcc
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    if (ENABLE_CPP_20)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines")
    endif()
    #-ftree-slp-vectorize with coroutine cause link error. disable it util gcc fix.
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fno-tree-slp-vectorize")
endif()
# --------------------- Clang

# --------------------- Msvc
# Resolves C1128 complained by MSVC: number of sections exceeded object file format limit: compile with /bigobj.
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/bigobj>")
# Resolves C4737 complained by MSVC: C4737: Unable to perform required tail call. Performance may be degraded. "Release-Type only"
if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/EHa>")
endif()