add_executable(struct_json_example
        main.cpp
        )
target_link_libraries(struct_json_example PRIVATE struct_json)

## manual import struct_json
include_directories(${yaLanTingLibs_SOURCE_DIR}/thirdparty/frozen)
include_directories(${yaLanTingLibs_SOURCE_DIR}/thirdparty/iguana)