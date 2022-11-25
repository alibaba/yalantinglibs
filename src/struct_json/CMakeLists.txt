add_library(libstruct_json INTERFACE)
target_include_directories(libstruct_json INTERFACE
        $<BUILD_INTERFACE:${yaLanTingLibs_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        )

target_link_libraries(libstruct_json INTERFACE iguana)