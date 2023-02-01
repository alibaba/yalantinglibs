# 3rd-party package load
option(USE_CONAN "Use conan package manager" OFF)
if(USE_CONAN)
    message(STATUS "Use conan package manager")
    find_package(asio REQUIRED)
else()
    add_subdirectory(${yaLanTingLibs_SOURCE_DIR}/thirdparty)
endif()
