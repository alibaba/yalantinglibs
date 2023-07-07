set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/examples/coro_rpc)
add_subdirectory(base_examples)
add_subdirectory(file_transfer)

if (CORO_RPC_USE_OTHER_RPC)
  add_subdirectory(user_defined_rpc_protocol/rest_rpc)
endif()