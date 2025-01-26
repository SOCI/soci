include(soci_define_backend_target)

if (SOCI_SQLITE3_AUTO)
  set(DEPENDENCY_MODE "DISABLE")
else()
  set(DEPENDENCY_MODE "ERROR")
endif()

soci_define_backend_target(
  BACKEND_NAME "SQLite3"
  TARGET_NAME "soci_sqlite3"
  ALIAS_NAME "SQLite3"
  DEPENDENCIES
    "SQLite3 YIELDS SQLite::SQLite3"
  REQUIRED_COMPONENTS SOCI::Core
  SOURCE_FILES
    "blob.cpp"
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
    "${PROJECT_SOURCE_DIR}/include/soci/sqlite3/soci-sqlite3.h"
  PRIVATE_INCLUDE_DIRS
    "${PROJECT_SOURCE_DIR}/include/private"
  MISSING_DEPENDENCY_BEHAVIOR "${DEPENDENCY_MODE}"
  ENABLED_VARIABLE "SOCI_SQLITE3"
)

if (NOT SOCI_SQLITE3)
  return()
endif()
