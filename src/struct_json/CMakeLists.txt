add_library(struct_json INTERFACE)
if (NOT TARGET yalantinglibs::struct_json)
    add_library(yalantinglibs::struct_json ALIAS struct_json)
endif ()
target_include_directories(struct_json INTERFACE
        $<BUILD_INTERFACE:${yaLanTingLibs_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/yalantinglibs>
        )

target_link_libraries(struct_json INTERFACE $<BUILD_INTERFACE:iguana>)
ylt_install(struct_json)
install(DIRECTORY "${yaLanTingLibs_SOURCE_DIR}/include/struct_json" DESTINATION include/yalantinglibs)
