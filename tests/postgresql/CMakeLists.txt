add_executable(soci_postgresql_test
  "test-postgresql.cpp"
)
target_link_libraries(soci_postgresql_test PRIVATE soci_tests_common SOCI::PostgreSQL)

set(SOCI_POSTGRESQL_TEST_CONNSTR "dbname=soci_test" CACHE STRING "The connection string to use for PostgreSQL tests")

add_test(
  NAME soci_postgresql_test
  COMMAND soci_postgresql_test "${SOCI_POSTGRESQL_TEST_CONNSTR}" "--invisibles"
)
