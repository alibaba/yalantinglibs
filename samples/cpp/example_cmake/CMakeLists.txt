# cmake needs this line
cmake_minimum_required(VERSION 3.1)

# Define project name
project(ylt_example_project)

# Find OpenCV, you may need to set OpenCV_DIR variable
# to the absolute path to the directory containing OpenCVConfig.cmake file
# via the command line or GUI
find_package(YaLanTing REQUIRED)

# If the package has been found, several variables will
# be set, you can find the full list with descriptions
# in the OpenCVConfig.cmake file.
# Print some message showing some of them
message(STATUS "YaLanTing library status:")
message(STATUS "    config: ${YaLanTing_DIR}")
message(STATUS "    version: ${YaLanTing_VERSION}")
message(STATUS "    libraries: ${YaLanTing_LIBS}")
message(STATUS "    include path: ${YaLanTing_INCLUDE_DIRS}")

# Declare the executable target built from your sources
add_executable(opencv_example example.cpp)

# Link your application with OpenCV libraries
target_link_libraries(ylt_example PRIVATE ${YaLanTing_LIBS})
target_include_directories(ylt_example PUBLIC ${YaLanTing_INCLUDE_DIRS})
