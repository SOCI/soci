soci_backend(Oracle
  DEPENDS Oracle
  HEADERS soci-oracle.h error.h
  DESCRIPTION "SOCI backend for Oracle 10+"
  AUTHORS "Maciej Sobczak, Stephen Hutton"
  MAINTAINERS "Maciej Sobczak")

# TODO: check the flag inside soci_backend_test macro (to be implemented) --mloskot
if (SOCI_TESTS)
  add_subdirectory(test)
endif()
