set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/examples)
add_subdirectory(helloworld)
add_subdirectory(file_transfer)

option(CORO_RPC_USE_OTHER_RPC "coro_rpc extend to support other rpc" OFF)
if (CORO_RPC_USE_OTHER_RPC)
  add_subdirectory(user_defined_rpc_protocol/rest_rpc)
endif()