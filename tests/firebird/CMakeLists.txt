add_library(firebird_tests OBJECT firebird_tests.cpp)
target_link_libraries(firebird_tests PUBLIC soci_common_tests soci_firebird_interface)

soci_make_tests(
  OBJECT_LIB        firebird_tests
  CONNECTION_STRING "service=/tmp/soci_test.fdb user=SYSDBA password=masterkey"
  SHARED_NAME       "soci_firebird_test"
  STATIC_NAME       "soci_firebird_test_static"
  SOCI_DEP_ALIAS    "Firebird"
)
