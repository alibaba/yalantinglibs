# install public header files
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "${yaLanTingLibs_BINARY_DIR}/cmake/yalantinglibsConfigVersion.cmake"
        VERSION ${yaLanTingLibs_VERSION}
        COMPATIBILITY AnyNewerVersion
)
set(ConfigPackageLocation lib/cmake/yalantinglibs)

configure_file(cmake/yalantinglibsConfig.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/cmake/yalantinglibsConfig.cmake"
        COPYONLY
        )
#install(DIRECTORY "${yaLanTingLibs_SOURCE_DIR}/include/" DESTINATION include/yalantinglibs)

install(
        FILES
        "${CMAKE_CURRENT_BINARY_DIR}/cmake/yalantinglibsConfig.cmake"
        "${yaLanTingLibs_BINARY_DIR}/cmake/yalantinglibsConfigVersion.cmake"
        DESTINATION ${ConfigPackageLocation}
)
function(ylt_install target)
    cmake_parse_arguments(ylt_install_lib " " "NAME" "EXPORT" ${ARGN})
    set(_export_name ${target}Targets)
    if (ylt_install_lib_NAME)
        set(_export_name ${ylt_install_lib_NAME})
    endif ()
    install(
            TARGETS ${target}
            EXPORT ${_export_name}
            LIBRARY DESTINATION lib
            ARCHIVE DESTINATION lib
            RUNTIME DESTINATION bin
    )
    include(CMakePackageConfigHelpers)
    write_basic_package_version_file(
            ${target}ConfigVersion.cmake
            VERSION ${yaLanTingLibs_VERSION}
            COMPATIBILITY SameMajorVersion
    )
    install(
            EXPORT ${_export_name}
            FILE ${target}Targets.cmake
            NAMESPACE yalantinglibs::
            DESTINATION ${ConfigPackageLocation}
    )
endfunction()