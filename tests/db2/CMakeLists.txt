add_library(db2_tests OBJECT db2_tests.cpp)
target_link_libraries(db2_tests PUBLIC soci_common_tests soci_db2_interface)

soci_make_tests(
  OBJECT_LIB        db2_tests
  CONNECTION_STRING "DSN=SAMPLE;Uid=db2inst1;Pwd=db2inst1;autocommit=off"
  SHARED_NAME       "soci_db2_test"
  STATIC_NAME       "soci_db2_test_static"
  SOCI_DEP_ALIAS    "DB2"
)
