include(soci_define_backend_target)

if (SOCI_ODBC_AUTO)
  set(DEPENDENCY_MODE "DISABLE")
else()
  set(DEPENDENCY_MODE "ERROR")
endif()

soci_define_backend_target(
  BACKEND_NAME "ODBC"
  TARGET_NAME "soci_odbc"
  ALIAS_NAME "ODBC"
  DEPENDENCIES
    "ODBC YIELDS ODBC::ODBC"
  REQUIRED_COMPONENTS SOCI::Core
  SOURCE_FILES
    "blob.cpp"
    "factory.cpp"
    "row-id.cpp"
    "session.cpp"
    "standard-into-type.cpp"
    "standard-use-type.cpp"
    "statement.cpp"
    "vector-into-type.cpp"
    "vector-use-type.cpp"
  HEADER_FILES
    "${PROJECT_SOURCE_DIR}/include/soci/odbc/soci-odbc.h"
  PRIVATE_INCLUDE_DIRS
    "${PROJECT_SOURCE_DIR}/include/private"
  MISSING_DEPENDENCY_BEHAVIOR "${DEPENDENCY_MODE}"
  ENABLED_VARIABLE "SOCI_ODBC"
)

if (NOT SOCI_ODBC)
  return()
endif()
