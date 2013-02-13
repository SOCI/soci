if (WIN32)
  # MDBTools driver seems unreliable
  soci_backend_test(
    NAME access
    BACKEND ODBC
    SOURCE test-odbc-access.cpp
    CONNSTR "test-access.dsn")

  # We have no means to test SQL Server at travis-ci.org
  soci_backend_test(
    NAME mssql
    BACKEND ODBC
    SOURCE test-odbc-mssql.cpp
    CONNSTR "test-mssql.dsn")
endif()

soci_backend_test(
  NAME mysql
  BACKEND ODBC
  SOURCE test-odbc-mysql.cpp
  CONNSTR "test-mysql.dsn")

soci_backend_test(
  NAME postgresql
  BACKEND ODBC
  SOURCE test-odbc-postgresql.cpp
  CONNSTR "test-postgresql.dsn")
