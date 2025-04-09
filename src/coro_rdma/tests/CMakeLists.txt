if(HAVE_IBVERBS_IBV)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/tests)
  add_executable(rdma_test
        test_device.cpp
        main.cpp
  )
  target_link_libraries(rdma_test -libverbs)
  add_test(NAME rdma_test COMMAND rdma_test)
endif()

