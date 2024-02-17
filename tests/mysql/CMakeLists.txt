add_library(mysql_tests OBJECT mysql_tests.cpp)
target_link_libraries(mysql_tests PUBLIC soci_common_tests soci_mysql_interface)

set(SOCI_MYSQL_TEST_CONNSTR "db=soci_test" CACHE STRING "The connection string to use for MySQL tests")

soci_make_tests(
  OBJECT_LIB        mysql_tests
  CONNECTION_STRING "${SOCI_MYSQL_TEST_CONNSTR}"
  SHARED_NAME       "soci_mysql_test"
  STATIC_NAME       "soci_mysql_test_static"
  SOCI_DEP_ALIAS    "MySQL"
)
