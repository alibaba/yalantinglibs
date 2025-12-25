
include(CheckCSourceCompiles)
set(CMAKE_REQUIRED_LIBRARIES accl_barex)
check_cxx_source_compiles("
    #define CMAKE_INCLUDE
    #include <accl/barex/xlistener.h>
    int main() {
        accl::barex::XListener* listener = nullptr;
        accl::barex::BarexResult result = accl::barex::XListener::NewInstance(
            listener, 1, 8000, accl::barex::TIMER_30S, {});
        return 0;
    }
" YLT_HAVE_BAREX_SUPPORT)
unset(CMAKE_REQUIRED_LIBRARIES)
unset(CMAKE_REQUIRED_INCLUDES)
if (YLT_HAVE_BAREX_SUPPORT)
    message(STATUS "Found ACCL barex support")
    SET(YLT_ENABLE_BAREX ON)
else()
    message(STATUS "ACCL barexsupport not found")
endif()

    