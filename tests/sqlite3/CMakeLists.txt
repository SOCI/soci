add_library(sqlite_tests OBJECT sqlite3_tests.cpp)
target_link_libraries(sqlite_tests PUBLIC soci_common_tests soci_sqlite3_interface)

set(SOCI_SQLITE3_TEST_CONNSTR ":memory:" CACHE STRING "The connection string to use for SQLite3 tests")

soci_make_tests(
  OBJECT_LIB        sqlite_tests
  CONNECTION_STRING "${SOCI_SQLITE3_TEST_CONNSTR}"
  SHARED_NAME       "soci_sqlite3_test"
  STATIC_NAME       "soci_sqlite3_test_static"
  SOCI_DEP_ALIAS    "SQLite3"
)

