soci_backend(PostgreSQL
  DEPENDS PostgreSQL
  HEADERS soci-postgresql.h common.h
  DESCRIPTION "SOCI backend for PostgreSQL database engine"
  AUTHORS "Maciej Sobczak, Stephen Hutton"
  MAINTAINERS "Maciej Sobczak")

add_subdirectory(test)
