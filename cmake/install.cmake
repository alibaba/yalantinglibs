include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "${yaLanTingLibs_BINARY_DIR}/cmake/yalantinglibsConfigVersion.cmake"
        VERSION ${yaLanTingLibs_VERSION}
        COMPATIBILITY AnyNewerVersion
)
set(ConfigPackageLocation lib/cmake/yalantinglibs)

install (DIRECTORY "${yaLanTingLibs_SOURCE_DIR}/share/" DESTINATION share)

install(DIRECTORY "${yaLanTingLibs_SOURCE_DIR}/include/" DESTINATION include REGEX "${yaLanTingLibs_SOURCE_DIR}/include/ylt/thirdparty" EXCLUDE)
message(STATUS "-------------INSTALL SETTING-------------")
option(INSTALL_THIRDPARTY "Install thirdparty" ON)
message(STATUS "INSTALL_THIRDPARTY: " ${INSTALL_THIRDPARTY})
option(INSTALL_INDEPENDENT_THIRDPARTY "Install independent thirdparty" OFF)
if (INSTALL_THIRDPARTY)
        message(STATUS "INSTALL_INDEPENDENT_THIRDPARTY: " ${INSTALL_INDEPENDENT_THIRDPARTY})
        if (INSTALL_INDEPENDENT_THIRDPARTY)
                install(DIRECTORY "${yaLanTingLibs_SOURCE_DIR}/include/ylt/thirdparty/" DESTINATION include)
        else()
                install(DIRECTORY "${yaLanTingLibs_SOURCE_DIR}/include/ylt/thirdparty/" DESTINATION include/ylt/thirdparty)
        endif()
endif()