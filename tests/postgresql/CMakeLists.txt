add_library(postgresql_tests OBJECT postgresql_tests.cpp)
target_link_libraries(postgresql_tests PUBLIC soci_common_tests soci_postgresql_interface)

set(SOCI_POSTGRESQL_TEST_CONNSTR "dbname=soci_test" CACHE STRING "The connection string to use for PostgreSQL tests")

soci_make_tests(
  OBJECT_LIB        postgresql_tests
  CONNECTION_STRING "${SOCI_POSTGRESQL_TEST_CONNSTR}"
  SHARED_NAME       "soci_postgresql_test"
  STATIC_NAME       "soci_postgresql_test_static"
  SOCI_DEP_ALIAS    "PostgreSQL"
)
