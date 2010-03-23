# TODO: to be refactored as a macro similar to soci_backend()
#       or as actual part of soci_backend()

set(SOCI_POSTGRESQL_TEST_CONNSTR ""
  CACHE STRING "Test connection string for PostgreSQL tests run")

set(SOCI_POSTGRESQL_TEST_TARGET test_postgresql)
set(SOCI_POSTGRESQL_TEST_SOURCES test-postgresql.cpp)

include_directories(${SOCI_SOURCE_DIR}/core/test)
include_directories(${SOCI_SOURCE_DIR}/backends/postgresql)

add_executable(${SOCI_POSTGRESQL_TEST_TARGET} ${SOCI_POSTGRESQL_TEST_SOURCES})
add_executable(${SOCI_POSTGRESQL_TEST_TARGET}_static ${SOCI_POSTGRESQL_TEST_SOURCES})

target_link_libraries(${SOCI_POSTGRESQL_TEST_TARGET}
  ${SOCI_CORE_TARGET}
  ${SOCI_POSTGRESQL_TARGET}
  ${POSTGRESQL_LIBRARIES})

target_link_libraries(${SOCI_POSTGRESQL_TEST_TARGET}_static
  ${SOCI_CORE_TARGET}-static
  ${SOCI_POSTGRESQL_TARGET}-static
  ${POSTGRESQL_LIBRARIES}
  ${SOCI_CORE_STATIC_DEPENDENCIES})

add_test(${SOCI_POSTGRESQL_TEST_TARGET}
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${SOCI_POSTGRESQL_TEST_TARGET}
  "${SOCI_POSTGRESQL_TEST_CONNSTR}")

add_test(${SOCI_POSTGRESQL_TEST_TARGET}_static
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${SOCI_POSTGRESQL_TEST_TARGET}_static
  "${SOCI_POSTGRESQL_TEST_CONNSTR}")
