add_library(struct_yaml INTERFACE)
if (NOT TARGET yalantinglibs::struct_yaml)
    add_library(yalantinglibs::struct_yaml ALIAS struct_yaml)
endif ()
target_include_directories(struct_yaml INTERFACE
        $<BUILD_INTERFACE:${yaLanTingLibs_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/yalantinglibs>
        )

target_link_libraries(struct_yaml INTERFACE $<BUILD_INTERFACE:iguana>)
ylt_install(struct_yaml)
install(DIRECTORY "${yaLanTingLibs_SOURCE_DIR}/include/struct_yaml" DESTINATION include/yalantinglibs)
