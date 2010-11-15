soci_backend(ODBC
  DEPENDS ODBC
  HEADERS soci-odbc.h utility.h
  DESCRIPTION "SOCI backend for ODBC"
  AUTHORS "Maciej Sobczak, Stephen Hutton, David Courtney"
  MAINTAINERS "Maciej Sobczak, Mateusz Loskot")

add_subdirectory(test)
