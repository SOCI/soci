# TODO: to be refactored as a macro similar to soci_backend()
#       or as actual part of soci_backend()

if(SOCI_EMPTY)

  set(SOCI_EMPTY_TEST_TARGET test_empty)
  set(SOCI_EMPTY_TEST_SOURCES test-empty.cpp)

  include_directories(${SOCI_SOURCE_DIR}/core/test)
  include_directories(${SOCI_SOURCE_DIR}/backends/empty)

  add_executable(${SOCI_EMPTY_TEST_TARGET} ${SOCI_EMPTY_TEST_SOURCES})
  add_executable(${SOCI_EMPTY_TEST_TARGET}_static ${SOCI_EMPTY_TEST_SOURCES})

  target_link_libraries(${SOCI_EMPTY_TEST_TARGET}
    ${SOCI_CORE_TARGET}
    ${SOCI_EMPTY_TARGET}
    ${EMPTY_LIBRARIES})

  target_link_libraries(${SOCI_EMPTY_TEST_TARGET}_static
    ${SOCI_CORE_TARGET}-static
    ${SOCI_EMPTY_TARGET}-static
    ${EMPTY_LIBRARIES}
    ${SOCI_CORE_STATIC_DEPENDENCIES})

  add_test(${SOCI_EMPTY_TEST_TARGET}
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${SOCI_EMPTY_TEST_TARGET}
    "dummyconnstr")

  add_test(${SOCI_EMPTY_TEST_TARGET}_static
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${SOCI_EMPTY_TEST_TARGET}_static
    "dummyconnstr")

endif()