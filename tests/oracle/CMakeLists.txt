add_library(oracle_tests OBJECT oracle_tests.cpp)
target_link_libraries(oracle_tests PUBLIC soci_common_tests soci_oracle_interface)

set(SOCI_ORACLE_TEST_CONNSTR "service=orcl user=scott password=tiger" CACHE STRING "The connection string to use for Oracle tests")

soci_make_tests(
  OBJECT_LIB        oracle_tests
  CONNECTION_STRING "${SOCI_ORACLE_TEST_CONNSTR}"
  SHARED_NAME       "soci_oracle_test"
  STATIC_NAME       "soci_oracle_test_static"
  SOCI_DEP_ALIAS    "Oracle"
)
