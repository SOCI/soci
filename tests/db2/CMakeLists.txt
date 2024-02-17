add_library(db2_tests OBJECT db2_tests.cpp)
target_link_libraries(db2_tests PUBLIC soci_common_tests soci_db2_interface)

set(SOCI_DB2_TEST_CONNSTR "DSN=SAMPLE;Uid=db2inst1;Pwd=db2inst1;autocommit=off" CACHE STRING "The connection string to use for DB2 tests")

soci_make_tests(
  OBJECT_LIB        db2_tests
  CONNECTION_STRING "${SOCI_DB2_TEST_CONNSTR}"
  SHARED_NAME       "soci_db2_test"
  STATIC_NAME       "soci_db2_test_static"
  SOCI_DEP_ALIAS    "DB2"
)
