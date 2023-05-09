add_library(struct_xml INTERFACE)
if (NOT TARGET yalantinglibs::struct_xml)
    add_library(yalantinglibs::struct_xml ALIAS struct_xml)
endif ()
target_include_directories(struct_xml INTERFACE
        $<BUILD_INTERFACE:${yaLanTingLibs_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/yalantinglibs>
        )

target_link_libraries(struct_xml INTERFACE $<BUILD_INTERFACE:iguana>)
ylt_install(struct_xml)
install(DIRECTORY "${yaLanTingLibs_SOURCE_DIR}/include/struct_xml" DESTINATION include/yalantinglibs)
