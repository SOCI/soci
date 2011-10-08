soci_backend(MySQL
  DEPENDS MySQL
  HEADERS soci-mysql.h common.h
  DESCRIPTION "SOCI backend for MySQL database engine"
  AUTHORS "Pawel Aleksander Fedorynski"
  MAINTAINERS "Pawel Aleksander Fedorynski")

add_subdirectory(test)
