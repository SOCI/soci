add_executable(soci_oracle_test
  "test-oracle.cpp"
)
target_link_libraries(soci_oracle_test PRIVATE soci_tests_common SOCI::Oracle)

set(SOCI_ORACLE_TEST_CONNSTR "service=orcl user=scott password=tiger" CACHE STRING "The connection string to use for Oracle tests")

add_test(
  NAME soci_oracle_test
  COMMAND soci_oracle_test "${SOCI_ORACLE_TEST_CONNSTR}" "--invisibles"
)
