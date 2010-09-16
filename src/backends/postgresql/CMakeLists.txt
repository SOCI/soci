soci_backend(PostgreSQL
  DEPENDS PostgreSQL
  HEADERS soci-postgresql.h common.h
  DESCRIPTION "SOCI backend for PostgreSQL database engine"
  AUTHORS "Maciej Sobczak, Stephen Hutton"
  MAINTAINERS "Maciej Sobczak")

# TODO: check the flag inside soci_backend_test macro (to be implemented) --mloskot
if (SOCI_TESTS)
  add_subdirectory(test)
endif()
