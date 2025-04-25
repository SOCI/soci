include(soci_define_backend_target)

if (SOCI_MYSQL_AUTO)
  set(DEPENDENCY_MODE "DISABLE")
else()
  set(DEPENDENCY_MODE "ERROR")
endif()

soci_define_backend_target(
  NAME "MySQL"
  TARGET_NAME "soci_mysql"
  DEPENDENCIES
    "MySQL YIELDS MySQL::MySQL"
  SOURCE_FILES
    "blob.cpp"
    "common.cpp"
    "error.cpp"
    "factory.cpp"
    "row-id.cpp"
    "session.cpp"
    "standard-into-type.cpp"
    "standard-use-type.cpp"
    "statement.cpp"
    "vector-into-type.cpp"
    "vector-use-type.cpp"
  HEADER_FILES
    "${PROJECT_SOURCE_DIR}/include/soci/mysql/soci-mysql.h"
  FIND_PACKAGE_FILES
    "${PROJECT_SOURCE_DIR}/cmake/find_modules/FindMySQL.cmake"
  MISSING_DEPENDENCY_BEHAVIOR "${DEPENDENCY_MODE}"
)

if (NOT SOCI_MYSQL)
  return()
endif()
