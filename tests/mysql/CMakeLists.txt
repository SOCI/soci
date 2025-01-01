add_executable(soci_mysql_tests
  "test-mysql.cpp"
)
target_link_libraries(soci_mysql_tests PRIVATE soci_tests_common SOCI::MySQL)

set(SOCI_MYSQL_TEST_CONNSTR "db=soci_test" CACHE STRING "The connection string to use for MySQL tests")

add_test(
  NAME soci_mysql_tests
  COMMAND soci_mysql_tests "${SOCI_MYSQL_TEST_CONNSTR}" "--invisibles"
)
