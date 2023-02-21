add_library(easylog INTERFACE)
if (NOT TARGET yalantinglibs::easylog)
    add_library(yalantinglibs::easylog ALIAS easylog)
endif ()
target_include_directories(easylog INTERFACE
        $<BUILD_INTERFACE:${yaLanTingLibs_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/yalantinglibs>
        )
# target_link_libraries(easylog INTERFACE $<BUILD_INTERFACE:iguana>)
ylt_install(easylog)
install(DIRECTORY "${yaLanTingLibs_SOURCE_DIR}/include/easylog" DESTINATION include/yalantinglibs)
