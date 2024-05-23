message(STATUS "-------------YLT CONFIG SETTING-------------")
option(YLT_ENABLE_SSL "Enable ssl support" ON)
message(STATUS "YLT_ENABLE_SSL: ${YLT_ENABLE_SSL}")
if (YLT_ENABLE_SSL)
    find_package(OpenSSL REQUIRED)
    if(CMAKE_PROJECT_NAME STREQUAL "yaLanTingLibs")
        add_compile_definitions("YLT_ENABLE_SSL")
        link_libraries(OpenSSL::SSL OpenSSL::Crypto)
    else ()
        target_compile_definitions(yalantinglibs INTERFACE "YLT_ENABLE_SSL")
        target_link_libraries(yalantinglibs INTERFACE OpenSSL::SSL OpenSSL::Crypto)
    endif ()
endif ()

option(YLT_ENABLE_PMR "Enable pmr support" OFF)
message(STATUS "YLT_ENABLE_PMR: ${YLT_ENABLE_PMR}")
if (YLT_ENABLE_PMR)
    if(CMAKE_PROJECT_NAME STREQUAL "yaLanTingLibs")
        add_compile_definitions("YLT_ENABLE_PMR" IGUANA_ENABLE_PMR)
    else ()
        target_compile_definitions(yalantinglibs INTERFACE "YLT_ENABLE_PMR" IGUANA_ENABLE_PMR)
    endif ()
endif ()

option(YLT_ENABLE_IO_URING "Enable io_uring" OFF)
message(STATUS "YLT_ENABLE_IO_URING: ${YLT_ENABLE_IO_URING}")
if (YLT_ENABLE_IO_URING)
  find_package(uring REQUIRED)
  message(STATUS "Use IO_URING for all I/O in linux")
  if(CMAKE_PROJECT_NAME STREQUAL "yaLanTingLibs")
      add_compile_definitions(ASIO_HAS_IO_URING ASIO_DISABLE_EPOLL ASIO_HAS_FILE YLT_ENABLE_FILE_IO_URING)
      link_libraries(uring)
  else ()
      target_compile_definitions(yalantinglibs INTERFACE ASIO_HAS_IO_URING ASIO_DISABLE_EPOLL ASIO_HAS_FILE YLT_ENABLE_FILE_IO_URING)
      target_link_libraries(yalantinglibs INTERFACE uring)
  endif ()
endif()

option(YLT_ENABLE_FILE_IO_URING "Enable file io_uring" OFF)
if (NOT YLT_ENABLE_IO_URING)
  if(YLT_ENABLE_FILE_IO_URING)
    find_package(uring REQUIRED)
    message(STATUS "YLT: Enable io_uring for file I/O in linux")
    if(CMAKE_PROJECT_NAME STREQUAL "yaLanTingLibs")
        add_compile_definitions(ASIO_HAS_IO_URING ASIO_HAS_FILE "YLT_ENABLE_FILE_IO_URING")
        link_libraries(uring)
    else ()
        target_compile_definitions(yalantinglibs INTERFACE ASIO_HAS_IO_URING ASIO_HAS_FILE "YLT_ENABLE_FILE_IO_URING")
        target_link_libraries(yalantinglibs INTERFACE uring)
    endif ()
  endif()
endif()

option(YLT_ENABLE_STRUCT_PACK_UNPORTABLE_TYPE "enable struct_pack unportable type(like wchar_t)" OFF)
message(STATUS "YLT_ENABLE_STRUCT_PACK_UNPORTABLE_TYPE: ${YLT_ENABLE_STRUCT_PACK_UNPORTABLE_TYPE}")
if(YLT_ENABLE_STRUCT_PACK_UNPORTABLE_TYPE)
  add_compile_definitions(STRUCT_PACK_ENABLE_UNPORTABLE_TYPE)
endif()

option(YLT_ENABLE_STRUCT_PACK_OPTIMIZE "enable struct_pack optimize(but cost more compile time)" OFF)
message(STATUS "YLT_ENABLE_STRUCT_PACK_OPTIMIZE: ${YLT_ENABLE_STRUCT_PACK_OPTIMIZE}")
if(YLT_ENABLE_STRUCT_PACK_OPTIMIZE)
    if(CMAKE_PROJECT_NAME STREQUAL "yaLanTingLibs")
        add_compile_definitions(YLT_ENABLE_STRUCT_PACK_OPTIMIZE)
    else ()
        target_compile_definitions(yalantinglibs INTERFACE YLT_ENABLE_STRUCT_PACK_OPTIMIZE)
    endif ()
endif()
message(STATUS "--------------------------------------------")