set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/examples/coro_rpc)
add_executable(coro_rpc_rdma_example
    rdma_example.cpp
  )
if (YLT_ENABLE_CUDA)
add_executable(coro_rpc_gdr_example
    gdr_example.cpp
  )
endif()