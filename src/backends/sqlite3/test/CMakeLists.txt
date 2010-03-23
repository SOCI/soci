# TODO: to be refactored as a macro similar to soci_backend()
#       or as actual part of soci_backend()

set(SOCI_SQLITE3_TEST_CONNSTR "test_sqlite2.db"
  CACHE STRING "Test connection string for SQLite3 tests run")

set(SOCI_SQLITE3_TEST_TARGET test_sqlite3)
set(SOCI_SQLITE3_TEST_SOURCES test-sqlite3.cpp)

include_directories(${SOCI_SOURCE_DIR}/core/test)
include_directories(${SOCI_SOURCE_DIR}/backends/sqlite3)

add_executable(${SOCI_SQLITE3_TEST_TARGET} ${SOCI_SQLITE3_TEST_SOURCES})
add_executable(${SOCI_SQLITE3_TEST_TARGET}_static ${SOCI_SQLITE3_TEST_SOURCES})

target_link_libraries(${SOCI_SQLITE3_TEST_TARGET}
  ${SOCI_CORE_TARGET}
  ${SOCI_SQLITE3_TARGET}
  ${SQLITE3_LIBRARIES})

target_link_libraries(${SOCI_SQLITE3_TEST_TARGET}_static
  ${SOCI_CORE_TARGET}-static
  ${SOCI_SQLITE3_TARGET}-static
  ${SQLITE3_LIBRARIES}
  ${SOCI_CORE_STATIC_DEPENDENCIES})

add_test(${SOCI_SQLITE3_TEST_TARGET}
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${SOCI_SQLITE3_TEST_TARGET}
  "${SOCI_SQLITE3_TEST_CONNSTR}")

add_test(${SOCI_SQLITE3_TEST_TARGET}_static
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${SOCI_SQLITE3_TEST_TARGET}_static
  "${SOCI_SQLITE3_TEST_CONNSTR}")
