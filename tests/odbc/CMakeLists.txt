set(MSSQL_VER $ENV{MSSQL_VER})
if (NOT MSSQL_VER)
    # Use the same value that was used before by default.
    set(MSSQL_VER "2014")
endif()
configure_file("test-mssql.dsn.in" "${CMAKE_CURRENT_BINARY_DIR}/test-mssql.dsn" @ONLY)


if (WIN32)
  add_library(odbc_ms_access_tests OBJECT odbc_ms_access_tests.cpp)
  target_link_libraries(odbc_ms_access_tests PUBLIC soci_common_tests soci_odbc_interface)

  soci_make_tests(
    OBJECT_LIB        odbc_ms_access_tests
    CONNECTION_STRING "FILEDSN=${CMAKE_CURRENT_BINARY_DIR}/test-access.dsn"
    SHARED_NAME       "soci_odbc_ms_access_test"
    STATIC_NAME       "soci_odbc_ms_access_test_static"
    SOCI_DEP_ALIAS    "ODBC"
  )
endif()


add_library(odbc_mssql_tests OBJECT odbc_mssql_tests.cpp)
target_link_libraries(odbc_mssql_tests PUBLIC soci_common_tests soci_odbc_interface)

soci_make_tests(
  OBJECT_LIB        odbc_mssql_tests
  CONNECTION_STRING "FILEDSN=${CMAKE_CURRENT_BINARY_DIR}/test-access.dsn"
  SHARED_NAME       "soci_odbc_mssql_test"
  STATIC_NAME       "soci_odbc_mssql_test_static"
  SOCI_DEP_ALIAS    "ODBC"
)


add_library(odbc_mysql_tests OBJECT odbc_mysql_tests.cpp)
target_link_libraries(odbc_mysql_tests PUBLIC soci_common_tests soci_odbc_interface)
target_include_directories(odbc_mysql_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../")

soci_make_tests(
  OBJECT_LIB        odbc_mysql_tests
  CONNECTION_STRING "FILEDSN=${CMAKE_CURRENT_SOURCE_DIR}/test-mysql.dsn"
  SHARED_NAME       "soci_odbc_mysql_test"
  STATIC_NAME       "soci_odbc_mysql_test_static"
  SOCI_DEP_ALIAS    "ODBC"
)


if(WIN32)
  set(TEST_PGSQL_DSN "test-postgresql-win64.dsn")
else()
  set(TEST_PGSQL_DSN "test-postgresql.dsn")
endif()

add_library(odbc_postgresql_tests OBJECT odbc_postgresql_tests.cpp)
target_link_libraries(odbc_postgresql_tests PUBLIC soci_common_tests soci_odbc_interface)

soci_make_tests(
  OBJECT_LIB        odbc_postgresql_tests
  CONNECTION_STRING "FILEDSN=${CMAKE_CURRENT_SOURCE_DIR}/${TEST_PGSQL_DSN}"
  SHARED_NAME       "soci_odbc_postgresql_test"
  STATIC_NAME       "soci_odbc_postgresql_test_static"
  SOCI_DEP_ALIAS    "ODBC"
)

# TODO: DB2 backend is tested by Travis CI on dedicated VM, separate from ODBC,
# in order to test DB2 with ODBC, it would be best to install DB2 driver only.
# if (NOT $ENV{TRAVIS})
option(WITH_ODBC_TEST_DB2 "Build ODBC DB2 test" OFF)
if (WITH_ODBC_TEST_DB2)
  add_library(odbc_db2_tests OBJECT odbc_db2_tests.cpp)
  target_link_libraries(odbc_db2_tests PUBLIC soci_common_tests soci_odbc_interface)

  soci_make_tests(
    OBJECT_LIB        odbc_db2_tests
    CONNECTION_STRING "FILEDSN=${CMAKE_CURRENT_SOURCE_DIR}/test-db2.dsn"
    SHARED_NAME       "soci_odbc_db2_test"
    STATIC_NAME       "soci_odbc_db2_test_static"
    SOCI_DEP_ALIAS    "ODBC"
  )
endif()
