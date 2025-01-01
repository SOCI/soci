add_executable(soci_postgresql_tests
  "test-postgresql.cpp"
)
target_link_libraries(soci_postgresql_tests PRIVATE soci_tests_common SOCI::PostgreSQL)

set(SOCI_POSTGRESQL_TEST_CONNSTR "dbname=soci_test" CACHE STRING "The connection string to use for PostgreSQL tests")

add_test(
  NAME soci_postgresql_tests
  COMMAND soci_postgresql_tests "${SOCI_POSTGRESQL_TEST_CONNSTR}" "--invisibles"
)
