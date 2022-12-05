#the thread library of the system.
find_package(Threads REQUIRED)

# 3rd-party package load
option(USE_CONAN "Use conan package manager" OFF)
if (USE_CONAN)
    message(STATUS "Use conan package manager")
    find_package(asio REQUIRED)
    find_package(spdlog REQUIRED)
else ()
    add_subdirectory(${CMAKE_SOURCE_DIR}/thirdparty)
endif ()