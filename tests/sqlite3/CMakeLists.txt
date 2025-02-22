add_executable(soci_sqlite3_test
  "test-sqlite3.cpp"
)
target_link_libraries(soci_sqlite3_test PRIVATE soci_tests_common SOCI::SQLite3)

set(SOCI_SQLITE3_TEST_CONNSTR ":memory:" CACHE STRING "The connection string to use for SQLite3 tests")

add_test(
  NAME soci_sqlite3_test
  COMMAND soci_sqlite3_test "${SOCI_SQLITE3_TEST_CONNSTR}" "--invisibles"
)
