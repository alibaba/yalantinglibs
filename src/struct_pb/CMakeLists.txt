add_library(libstruct_pb INTERFACE)
target_include_directories(libstruct_pb INTERFACE
        $<BUILD_INTERFACE:${yaLanTingLibs_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        )
