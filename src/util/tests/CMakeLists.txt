set(target_name util_tests)
set(headers_and_sources static_asserts.cpp)
add_executable(${target_name})
target_sources(${target_name} PRIVATE
    ${headers_and_sources}
)
target_include_directories(${target_name} PRIVATE
       ${yaLanTingLibs_SOURCE_DIR}/include
)
