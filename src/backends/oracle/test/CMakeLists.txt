# TODO: to be refactored as a macro similar to soci_backend()
#       or as actual part of soci_backend()

if(SOCI_ORACLE)

  set(SOCI_ORACLE_TEST_CONNSTR ""
    CACHE STRING "Test connection string for Oracle tests run")

  set(SOCI_ORACLE_TEST_TARGET test_oracle)
  set(SOCI_ORACLE_TEST_SOURCES test-oracle.cpp)

  include_directories(${SOCI_SOURCE_DIR}/core/test)
  include_directories(${SOCI_SOURCE_DIR}/backends/oracle)

  add_executable(${SOCI_ORACLE_TEST_TARGET} ${SOCI_ORACLE_TEST_SOURCES})
  add_executable(${SOCI_ORACLE_TEST_TARGET}_static ${SOCI_ORACLE_TEST_SOURCES})

  target_link_libraries(${SOCI_ORACLE_TEST_TARGET}
    ${SOCI_CORE_TARGET}
    ${SOCI_ORACLE_TARGET}
    ${ORACLE_LIBRARIES})

  target_link_libraries(${SOCI_ORACLE_TEST_TARGET}_static
    ${SOCI_CORE_TARGET}-static
    ${SOCI_ORACLE_TARGET}-static
    ${ORACLE_LIBRARIES}
    ${SOCI_CORE_STATIC_DEPENDENCIES})

  add_test(${SOCI_ORACLE_TEST_TARGET}
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${SOCI_ORACLE_TEST_TARGET})

  add_test(${SOCI_ORACLE_TEST_TARGET}_static
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${SOCI_ORACLE_TEST_TARGET}_static)

endif()