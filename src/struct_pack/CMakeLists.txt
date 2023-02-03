add_library(struct_pack INTERFACE)
if (NOT TARGET yalantinglibs::struct_pack)
    add_library(yalantinglibs::struct_pack ALIAS struct_pack)
endif ()
target_include_directories(struct_pack INTERFACE
        $<BUILD_INTERFACE:${yaLanTingLibs_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${yaLanTingLibs_SOURCE_DIR}/thirdparty/frozen>
        $<INSTALL_INTERFACE:include/yalantinglibs>
        )
#install(TARGETS struct_pack
#        EXPORT struct_packTargets
#        LIBRARY DESTINATION lib
#        ARCHIVE DESTINATION lib
#        RUNTIME DESTINATION bin
#        )
#export(EXPORT struct_packTargets
#        FILE "${yaLanTingLibs_BINARY_DIR}/cmake/struct_packTargets.cmake"
#        NAMESPACE yalantinglibs::
#        )
#install(EXPORT struct_packTargets
#        FILE struct_packTargets.cmake
#        NAMESPACE yalantinglibs::
#        DESTINATION ${ConfigPackageLocation}
#        )
ylt_install(struct_pack)
install(DIRECTORY "${yaLanTingLibs_SOURCE_DIR}/include/struct_pack" DESTINATION include/yalantinglibs)