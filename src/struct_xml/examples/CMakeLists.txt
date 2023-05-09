add_executable(struct_xml_example
        main.cpp
        )
target_link_libraries(struct_xml_example PRIVATE struct_xml)

## manual import struct_xml
include_directories(${yaLanTingLibs_SOURCE_DIR}/thirdparty/iguana)
include_directories(${yaLanTingLibs_SOURCE_DIR}/thirdparty/iguana/thirdparty/rapidxml)
include_directories(${yaLanTingLibs_SOURCE_DIR}/thirdparty/iguana/thirdparty/mscharconv/include)