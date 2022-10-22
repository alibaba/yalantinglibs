add_library(libstruct_pack INTERFACE)
target_include_directories(libstruct_pack INTERFACE
        $<BUILD_INTERFACE:${yaLanTingLibs_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        )
