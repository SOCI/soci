add_library(sqlite_tests OBJECT sqlite3_tests.cpp)
target_link_libraries(sqlite_tests PUBLIC soci_common_tests soci_sqlite3_interface)

soci_make_tests(
  OBJECT_LIB        sqlite_tests
  CONNECTION_STRING ":memory:"
  SHARED_NAME       "soci_sqlite3_test"
  STATIC_NAME       "soci_sqlite3_test_static"
  SOCI_DEP_ALIAS    "SQLite3"
)

