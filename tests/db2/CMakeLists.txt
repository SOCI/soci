add_executable(soci_db2_tests OBJECT db2_tests.cpp)
target_link_libraries(soci_db2_tests PRIVATE soci_common_tests soci_db2_interface)

set(SOCI_DB2_TEST_CONNSTR "DSN=SAMPLE;Uid=db2inst1;Pwd=db2inst1;autocommit=off" CACHE STRING "The connection string to use for DB2 tests")

add_test(
  NAME soci_db2_tests
  COMMAND soci_db2_tests "${SOCI_DB2_TEST_CONNSTR}" "--invisibles"
)
