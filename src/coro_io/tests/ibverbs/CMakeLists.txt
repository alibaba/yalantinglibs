if(HAVE_IBVERBS_IBV)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/tests)
  add_executable(ibverbs_test
  test_io.cpp
  )
  target_link_libraries(ibverbs_test -libverbs)
  add_test(NAME ibverbs_test COMMAND ibverbs_test)
endif()

