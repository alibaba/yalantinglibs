add_library(libcoro_rpc INTERFACE)
target_link_libraries(libcoro_rpc INTERFACE libstruct_pack)
target_include_directories(libcoro_rpc INTERFACE
        $<BUILD_INTERFACE:${yaLanTingLibs_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        )

target_link_libraries(libcoro_rpc INTERFACE
        Threads::Threads
        asio::asio
        async_simple::async_simple_header_only
        )

option(ENABLE_IO_URING "Enable io_uring" OFF)
if (ENABLE_IO_URING) 
    message(STATUS "Use IO_URING for I/O in linux")
    target_compile_definitions(libcoro_rpc INTERFACE ASIO_HAS_IO_URING)
    target_compile_definitions(libcoro_rpc INTERFACE ASIO_DISABLE_EPOLL)
    target_link_libraries(libcoro_rpc INTERFACE -luring)
endif()
option(ENABLE_SSL "Enable ssl support" OFF)
if (ENABLE_SSL)
    message(STATUS "Use SSL")
    find_package(OpenSSL REQUIRED)
    target_compile_definitions(libcoro_rpc INTERFACE ENABLE_SSL)
    target_link_libraries(libcoro_rpc INTERFACE OpenSSL::SSL OpenSSL::Crypto)
endif ()
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # When using coro_rpc_client, only remote function declarations are required.
    # Generally speaking, RPC function declaration and definition are divided.
    # However, clang + ld(gold linker) + '-g' will report 'undefined reference to xxx'.
    # We force use lld when compiler is clang.
    # Note: gcc + ld + '-g' or clang + lld + '-g' work well
    target_link_options(libcoro_rpc INTERFACE "-fuse-ld=lld")
    message(STATUS "Force add link option: -fuse-ld=lld")
endif ()