if(YLT_ENABLE_BAREX)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/tests)


  add_executable(barex_test
    main.cpp
    test_barex_socket.cpp
  )

  add_executable(barex_socket_pressure_test
    main.cpp
    barex_socket_pressure_test.cpp
  )

  target_link_libraries(barex_test /usr/lib64/libaccl_barex_static.a -lrt)
  target_link_libraries(barex_socket_pressure_test /usr/lib64/libaccl_barex_static.a -lrt)
  add_test(NAME barex_test COMMAND ibverbs_test)
  add_test(NAME barex_socket_pressure_test COMMAND barex_socket_pressure_test)
  set_property(TEST barex_test PROPERTY ENVIRONMENT "ACCL_SET_ERDMA=1")
  set_property(TEST barex_socket_pressure_test PROPERTY ENVIRONMENT "ACCL_SET_ERDMA=1")
endif()
