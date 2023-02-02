add_library(libeasylog INTERFACE)
target_include_directories(libeasylog INTERFACE
        $<BUILD_INTERFACE:${yaLanTingLibs_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        )