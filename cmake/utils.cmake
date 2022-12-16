macro(check_asan _RESULT)
    include(CheckCXXSourceRuns)
    set(CMAKE_REQUIRED_FLAGS "-fsanitize=address")
    check_cxx_source_runs(
            [====[
int main()
{
  return 0;
}
]====]
            ${_RESULT}
    )
    unset(CMAKE_REQUIRED_FLAGS)
endmacro()

# Enable address sanitizer for a target (gcc/clang only)
function(target_enable_sanitizer target_name)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        message(FATAL_ERROR "Sanitizer supported only for gcc/clang")
    endif()
    message(STATUS "Address sanitizer enabled")
    target_compile_options(${target_name} PRIVATE -fsanitize=address,undefined)
    target_compile_options(${target_name} PRIVATE -fno-sanitize=signed-integer-overflow)
    target_compile_options(${target_name} PRIVATE -fno-sanitize-recover=all)
    target_compile_options(${target_name} PRIVATE -fno-omit-frame-pointer)
    #    target_link_libraries(${target_name} PRIVATE -fsanitize=address,undefined -fuse-ld=gold) #
endfunction()

# Turn on warnings on the given target
function(target_enable_warnings target_name)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        list(APPEND MSVC_OPTIONS "/W3")
        if(MSVC_VERSION GREATER 1900) # Allow non fatal security warnings for msvc 2015
            list(APPEND MSVC_OPTIONS "/WX")
        endif()
    endif()
    target_compile_options(
            ${target_name}
            PRIVATE $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
            -Wall
            -Wextra
            -Wconversion
            -pedantic
            -Werror
            -Wfatal-errors>
            $<$<CXX_COMPILER_ID:MSVC>:${MSVC_OPTIONS}>)
endfunction()
