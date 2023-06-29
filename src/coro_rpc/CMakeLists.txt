add_library(coro_rpc INTERFACE)

if (NOT TARGET yalantinglibs::coro_rpc)
    add_library(yalantinglibs::coro_rpc ALIAS coro_rpc)
endif ()
target_link_libraries(coro_rpc INTERFACE struct_pack easylog)
target_include_directories(coro_rpc INTERFACE
        $<BUILD_INTERFACE:${yaLanTingLibs_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/yalantinglibs>
        )

target_link_libraries(coro_rpc INTERFACE
        Threads::Threads
        $<BUILD_INTERFACE:asio::asio>
        $<BUILD_INTERFACE:async_simple::async_simple_header_only>
        )

option(ENABLE_IO_URING "Enable io_uring" OFF)
if (ENABLE_IO_URING)
    message(STATUS "Use IO_URING for I/O in linux")
    target_compile_definitions(coro_rpc INTERFACE ASIO_HAS_IO_URING)
    target_compile_definitions(coro_rpc INTERFACE ASIO_DISABLE_EPOLL)
    target_link_libraries(coro_rpc INTERFACE -luring)
else()
    target_compile_definitions(coro_rpc INTERFACE ASIO_DISABLE_IO_URING)
endif()
option(ENABLE_SSL "Enable ssl support" OFF)
if (ENABLE_SSL)
    message(STATUS "Use SSL")
    find_package(OpenSSL REQUIRED)
    target_compile_definitions(coro_rpc INTERFACE ENABLE_SSL)
    target_link_libraries(coro_rpc INTERFACE OpenSSL::SSL OpenSSL::Crypto)
endif ()
option(ENABLE_PMR "Enable pmr support" OFF)
if (ENABLE_PMR)
    message(STATUS "Use std::pmr")
    target_compile_definitions(coro_rpc INTERFACE YLT_ENABLE_PMR)
endif ()
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # When using coro_rpc_client, only remote function declarations are required.
    # Generally speaking, RPC function declaration and definition are divided.
    # However, clang + ld(gold linker) + '-g' will report 'undefined reference to xxx'.
    # We force use lld when compiler is clang.
    # Note: gcc + ld + '-g' or clang + lld + '-g' work well
    target_link_options(coro_rpc INTERFACE "-fuse-ld=lld")
    message(STATUS "Force add link option: -fuse-ld=lld")
endif ()


ylt_install(coro_rpc)
install(DIRECTORY "${yaLanTingLibs_SOURCE_DIR}/include/coro_rpc" DESTINATION include/yalantinglibs)
install(DIRECTORY "${yaLanTingLibs_SOURCE_DIR}/include/coro_io" DESTINATION include/yalantinglibs)
install(DIRECTORY "${yaLanTingLibs_SOURCE_DIR}/include/util" DESTINATION include/yalantinglibs)

