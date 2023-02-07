add_library(struct_pb INTERFACE)
install(TARGETS struct_pb EXPORT struct_pb
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
        INCLUDES DESTINATION include
        )
if (NOT TARGET yalantinglibs::struct_pb)
    add_library(yalantinglibs::struct_pb ALIAS struct_pb)
endif ()
target_include_directories(struct_pb INTERFACE
        $<BUILD_INTERFACE:${yaLanTingLibs_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/yalantinglibs>
        )
add_subdirectory(protoc-plugin)
add_subdirectory(conformance)

ylt_install(struct_pb)
install(DIRECTORY "${yaLanTingLibs_SOURCE_DIR}/include/struct_pb" DESTINATION include/yalantinglibs)
install(FILES "${yaLanTingLibs_SOURCE_DIR}/cmake/struct_pb.cmake"
        DESTINATION ${ConfigPackageLocation}
        )
