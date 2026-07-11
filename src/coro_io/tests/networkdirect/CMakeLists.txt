# NetworkDirect (ND) backend is Windows-only; gated by YLT_ENABLE_ND.
if(YLT_ENABLE_ND)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/tests)

  add_executable(networkdirect_test
    test_nd_buffer_pool.cpp
    test_nd_socket.cpp
    main.cpp
  )
  target_link_libraries(networkdirect_test PRIVATE ws2_32 mswsock)

  add_test(NAME networkdirect_test COMMAND networkdirect_test)
endif()
