soci_backend(Oracle
  DEPENDS Oracle
  HEADERS soci-oracle.h error.h
  DESCRIPTION "SOCI backend for Oracle 10+"
  AUTHORS "Maciej Sobczak, Stephen Hutton"
  MAINTAINERS "Maciej Sobczak")

add_subdirectory(test)

