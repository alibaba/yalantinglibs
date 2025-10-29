#if(YLT_HAVE_IBVERBS)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/tests)


  add_executable(barex_test
    test.cpp
  )

  target_link_libraries(barex_test -laccl_barex)
#endif()
