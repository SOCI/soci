soci_backend_test(access1
  BACKEND ODBC
  SOURCE test-odbc-access.cpp
  CONNSTR "test-access.dsn")

soci_backend_test(postgresql1
  BACKEND ODBC
  SOURCE test-odbc-postgresql.cpp
  CONNSTR "test-postgresql.dsn")

