add_library(oracle_tests OBJECT oracle_tests.cpp)
target_link_libraries(oracle_tests PUBLIC soci_common_tests soci_oracle_interface)

soci_make_tests(
  OBJECT_LIB        oracle_tests
  CONNECTION_STRING "service=orcl user=scott password=tiger"
  SHARED_NAME       "soci_oracle_test"
  STATIC_NAME       "soci_oracle_test_static"
  SOCI_DEP_ALIAS    "Oracle"
)
