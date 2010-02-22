set(SOCI_TEST_SQLITE3_TARGET test_sqlite3)
set(SOCI_TEST_SQLITE3_SOURCES test-sqlite3.cpp)

include_directories(${SOCI_SOURCE_DIR}/core/test)
include_directories(${SOCI_SOURCE_DIR}/backends/sqlite3)

add_executable(${SOCI_TEST_SQLITE3_TARGET} ${SOCI_TEST_SQLITE3_SOURCES})

target_link_libraries(${SOCI_TEST_SQLITE3_TARGET}
  ${SOCI_CORE_TARGET}
  ${SOCI_BACKEND_SQLITE3_TARGET})
