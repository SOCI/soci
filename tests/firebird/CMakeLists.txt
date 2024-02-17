add_library(firebird_tests OBJECT firebird_tests.cpp)
target_link_libraries(firebird_tests PUBLIC soci_common_tests soci_firebird_interface)

set(SOCI_FIREBIRD_TEST_CONNSTR "service=/tmp/soci_test.fdb user=SYSDBA password=masterkey" CACHE STRING "The connection string to use for Firebird tests")

soci_make_tests(
  OBJECT_LIB        firebird_tests
  CONNECTION_STRING "${SOCI_FIREBIRD_TEST_CONNSTR}"
  SHARED_NAME       "soci_firebird_test"
  STATIC_NAME       "soci_firebird_test_static"
  SOCI_DEP_ALIAS    "Firebird"
)
