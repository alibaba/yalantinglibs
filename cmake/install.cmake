message(STATUS "-------------INSTALL SETTING-------------")
option(INSTALL_THIRDPARTY "Install thirdparty" ON)
message(STATUS "INSTALL_THIRDPARTY: " ${INSTALL_THIRDPARTY})
option(INSTALL_INDEPENDENT_THIRDPARTY "Install independent thirdparty" ON)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "${yaLanTingLibs_BINARY_DIR}/cmake/yalantinglibsConfigVersion.cmake"
        VERSION ${yaLanTingLibs_VERSION}
        COMPATIBILITY AnyNewerVersion
)
set(ConfigPackageLocation lib/cmake/yalantinglibs)

add_library(yalantinglibs INTERFACE)
add_library(yalantinglibs::yalantinglibs ALIAS yalantinglibs)

target_include_directories(yalantinglibs INTERFACE 
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../include/ylt/thirdparty>
)
install(TARGETS yalantinglibs
       EXPORT yalantinglibsTargets
       LIBRARY DESTINATION lib
       ARCHIVE DESTINATION lib
       RUNTIME DESTINATION bin
       )
install(EXPORT yalantinglibsTargets
       FILE yalantinglibsConfig.cmake
       NAMESPACE yalantinglibs::
       DESTINATION ${ConfigPackageLocation}
       )

install(DIRECTORY "${yaLanTingLibs_SOURCE_DIR}/include/" DESTINATION include REGEX "${yaLanTingLibs_SOURCE_DIR}/include/ylt/thirdparty" EXCLUDE)

if (INSTALL_THIRDPARTY)
        message(STATUS "INSTALL_INDEPENDENT_THIRDPARTY: " ${INSTALL_INDEPENDENT_THIRDPARTY})
        if (INSTALL_INDEPENDENT_THIRDPARTY)
                install(DIRECTORY "${yaLanTingLibs_SOURCE_DIR}/include/ylt/thirdparty/" DESTINATION include)
        else()
                install(DIRECTORY "${yaLanTingLibs_SOURCE_DIR}/include/ylt/thirdparty/" DESTINATION include/ylt/thirdparty)
                target_include_directories(yalantinglibs INTERFACE
                $<INSTALL_INTERFACE:include/ylt/thirdparty>
                )
        endif()
endif()