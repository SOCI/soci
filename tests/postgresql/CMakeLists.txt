add_library(postgresql_tests OBJECT postgresql_tests.cpp)
target_link_libraries(postgresql_tests PUBLIC soci_common_tests soci_postgresql_interface)

soci_make_tests(
  OBJECT_LIB        postgresql_tests
  CONNECTION_STRING "dbname=soci_test"
  SHARED_NAME       "soci_postgresql_test"
  STATIC_NAME       "soci_postgresql_test_static"
  SOCI_DEP_ALIAS    "PostgreSQL"
)
