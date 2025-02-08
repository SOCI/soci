set(MSSQL_VER $ENV{MSSQL_VER})
if (NOT MSSQL_VER)
    # Use the same value that was used before by default.
    set(MSSQL_VER "2014")
endif()
configure_file("test-mssql.dsn.in" "${CMAKE_CURRENT_BINARY_DIR}/test-mssql.dsn" @ONLY)


if (WIN32)
  add_executable(soci_odbc_test_ms_access
    "test-odbc-access.cpp"
  )
  target_link_libraries(soci_odbc_test_ms_access PRIVATE soci_tests_common SOCI::ODBC)

  set(SOCI_ODBC_TEST_ACCESS_CONNSTR "FILEDSN=${CMAKE_CURRENT_SOURCE_DIR}/test-access.dsn" CACHE STRING "Connection string for the ODBC MS Access test")

  add_test(
    NAME soci_odbc_test_ms_access
    COMMAND soci_odbc_test_ms_access "${SOCI_ODBC_TEST_ACCESS_CONNSTR}" "--invisibles"
  )
endif()


add_executable(soci_odbc_test_mssql
  "test-odbc-mssql.cpp"
)
target_link_libraries(soci_odbc_test_mssql PRIVATE soci_tests_common SOCI::ODBC)

set(SOCI_ODBC_TEST_MSSQL_CONNSTR "FILEDSN=${CMAKE_CURRENT_BINARY_DIR}/test-mssql.dsn" CACHE STRING "Connection string for the ODBC MSSQL test")

add_test(
  NAME soci_odbc_test_mssql
  COMMAND soci_odbc_test_mssql "${SOCI_ODBC_TEST_MSSQL_CONNSTR}" "--invisibles"
)


add_executable(soci_odbc_test_mysql
  "test-odbc-mysql.cpp"
)
target_link_libraries(soci_odbc_test_mysql PRIVATE soci_tests_common SOCI::ODBC)

set(SOCI_ODBC_TEST_MYSQL_CONNSTR "FILEDSN=${CMAKE_CURRENT_SOURCE_DIR}/test-mysql.dsn" CACHE STRING "Connection string for the ODBC MySQL test")

add_test(
  NAME soci_odbc_test_mysql
  COMMAND soci_odbc_test_mysql "${SOCI_ODBC_TEST_MYSQL_CONNSTR}" "--invisibles"
)


if(WIN32)
  set(TEST_PGSQL_DSN "test-postgresql-win64.dsn")
else()
  set(TEST_PGSQL_DSN "test-postgresql.dsn")
endif()

add_executable(soci_odbc_test_postgresql
  "test-odbc-postgresql.cpp"
)
target_link_libraries(soci_odbc_test_postgresql PRIVATE soci_tests_common SOCI::ODBC)

set(SOCI_ODBC_TEST_POSTGRESQL_CONNSTR "FILEDSN=${CMAKE_CURRENT_SOURCE_DIR}/${TEST_PGSQL_DSN}" CACHE STRING "Connection string for the ODBC PostgreSQL test")

add_test(
  NAME soci_odbc_test_postgresql
  COMMAND soci_odbc_test_postgresql "${SOCI_ODBC_TEST_POSTGRESQL_CONNSTR}" "--invisibles"
)


# TODO: DB2 backend is tested by Travis CI on dedicated VM, separate from ODBC,
# in order to test DB2 with ODBC, it would be best to install DB2 driver only.
# if (NOT $ENV{TRAVIS})
option(WITH_ODBC_TEST_DB2 "Build ODBC DB2 test" OFF)
if (WITH_ODBC_TEST_DB2)
  add_executable(soci_odbc_test_db2
    "test-odbc-db2.cpp"
  )
  target_link_libraries(osoci_dbc_db2_tests PRIVATE soci_tests_common SOCI::ODBC)

  set(SOCI_ODBC_TEST_DB2_CONNSTR "FILEDSN=${CMAKE_CURRENT_SOURCE_DIR}/test-db2.dsn" CACHE STRING "Connection string for the ODBC DB2 test")

  add_test(
    NAME soci_odbc_test_db2
    COMMAND soci_odbc_test_db2 "${SOCI_ODBC_TEST_DB2_CONNSTR}" "--invisibles"
  )
endif()
