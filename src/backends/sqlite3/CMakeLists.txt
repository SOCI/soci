soci_backend(SQLite3
  DEPENDS SQLite3
  HEADERS soci-sqlite3.h common.h
  DESCRIPTION "SOCI backend for SQLite 3 database engine"
  AUTHORS "Maciej Sobczak, Stephen Hutton, David Courtney"
  MAINTAINERS "Maciej Sobczak, Mateusz Loskot")

# TODO: check the flag inside soci_backend_test macro (to be implemented) --mloskot
if (SOCI_TESTS)
  add_subdirectory(test)
endif()
