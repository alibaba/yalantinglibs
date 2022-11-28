#############################
# asio
#############################
add_library(asio INTERFACE)
target_include_directories(asio INTERFACE asio)
add_definitions(asio INTERFACE ASIO_HAS_STD_INVOKE_RESULT)
if (NOT TARGET asio::asio)
    add_library(asio::asio ALIAS asio)
endif ()

#############################
# spdlog
#############################
add_library(spdlog INTERFACE)
target_include_directories(spdlog INTERFACE spdlog)
if (NOT TARGET spdlog::spdlog)
    add_library(spdlog::spdlog ALIAS spdlog)
endif ()

#############################
# libasync_simple
#############################
add_library(libasync_simple INTERFACE)
target_include_directories(libasync_simple INTERFACE async_simple)
target_compile_definitions(libasync_simple INTERFACE ASYNC_SIMPLE_HAS_NOT_AIO)
if (NOT TARGET async_simple::async_simple_header_only)
    add_library(async_simple::async_simple_header_only ALIAS libasync_simple)
endif ()

#############################
# doctest
#############################
add_library(doctest INTERFACE)
target_include_directories(doctest INTERFACE doctest)

#############################
# frozen
#############################
add_library(frozen INTERFACE)
target_include_directories(frozen INTERFACE frozen)

#############################
# iguana
#############################
add_library(iguana INTERFACE)
target_include_directories(iguana INTERFACE frozen iguana)
