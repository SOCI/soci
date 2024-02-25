add_executable(soci_sqlite3_tests sqlite3_tests.cpp)
target_link_libraries(soci_sqlite3_tests PRIVATE soci_common_tests SOCI::SQLite3)

set(SOCI_SQLITE3_TEST_CONNSTR ":memory:" CACHE STRING "The connection string to use for SQLite3 tests")

add_test(
  NAME soci_sqlite3_tests
  COMMAND soci_sqlite3_tests "${SOCI_SQLITE3_TEST_CONNSTR}" "--invisibles"
)
