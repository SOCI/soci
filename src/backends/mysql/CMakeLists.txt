soci_backend(MySQL
  DEPENDS MySQL
  HEADERS soci-mysql.h common.h
  DESCRIPTION "SOCI backend for MySQL database engine"
  AUTHORS "Pawel Aleksander Fedorynski"
  MAINTAINERS "Pawel Aleksander Fedorynski")

# TODO: check the flag inside soci_backend_test macro (to be implemented) --mloskot
if (SOCI_TESTS)
  add_subdirectory(test)
endif()
