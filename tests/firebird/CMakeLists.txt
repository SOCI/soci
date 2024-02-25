add_executable(soci_firebird_tests firebird_tests.cpp)
target_link_libraries(soci_firebird_tests PRIVATE soci_common_tests SOCI::Firebird)

set(SOCI_FIREBIRD_TEST_CONNSTR "service=/tmp/soci_test.fdb user=SYSDBA password=masterkey" CACHE STRING "The connection string to use for Firebird tests")

add_test(
  NAME soci_firebird_tests
  COMMAND soci_firebird_tests "${SOCI_FIREBIRD_TEST_CONNSTR}" "--invisibles"
)
