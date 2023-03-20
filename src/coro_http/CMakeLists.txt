add_library(coro_http INTERFACE)
if (NOT TARGET yalantinglibs::coro_http)
    add_library(yalantinglibs::coro_http ALIAS coro_http)
endif ()
target_include_directories(coro_http INTERFACE
        $<BUILD_INTERFACE:${yaLanTingLibs_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/yalantinglibs>
        )

target_link_libraries(coro_rpc INTERFACE
        Threads::Threads
        $<BUILD_INTERFACE:asio::asio>
        $<BUILD_INTERFACE:async_simple::async_simple_header_only>
        )

target_link_libraries(coro_http INTERFACE $<BUILD_INTERFACE:cinatra>)
option(ENABLE_SSL "Enable ssl support" OFF)
if (ENABLE_SSL)
    message(STATUS "Use SSL")
    find_package(OpenSSL REQUIRED)
    target_compile_definitions(coro_http INTERFACE CINATRA_ENABLE_SSL ENABLE_SSL)
    target_link_libraries(coro_http INTERFACE OpenSSL::SSL OpenSSL::Crypto)
endif ()

ylt_install(coro_http)
install(DIRECTORY "${yaLanTingLibs_SOURCE_DIR}/include/coro_http" DESTINATION include/yalantinglibs)
