add_executable(soci_db2_test
  "test-db2.cpp"
)
target_link_libraries(soci_db2_test PRIVATE soci_tests_common SOCI::DB2)

set(SOCI_DB2_TEST_CONNSTR "DSN=SAMPLE;Uid=db2inst1;Pwd=db2inst1;autocommit=off" CACHE STRING "The connection string to use for DB2 tests")

add_test(
  NAME soci_db2_test
  COMMAND soci_db2_test "${SOCI_DB2_TEST_CONNSTR}" "--invisibles"
)
