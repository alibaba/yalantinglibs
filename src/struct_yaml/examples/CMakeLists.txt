add_executable(struct_yaml_example
        main.cpp
        )
target_link_libraries(struct_yaml_example PRIVATE struct_yaml)

## manual import struct_yaml
include_directories(${yaLanTingLibs_SOURCE_DIR}/thirdparty/iguana)
include_directories(${yaLanTingLibs_SOURCE_DIR}/thirdparty/iguana/thirdparty/rapidxml)
