include(soci_define_backend_target)

if (SOCI_ODBC_AUTO)
  set(DEPENDENCY_MODE "DISABLE")
else()
  set(DEPENDENCY_MODE "ERROR")
endif()

soci_define_backend_target(
  NAME "ODBC"
  TARGET_NAME "soci_odbc"
  DEPENDENCIES
    "ODBC YIELDS ODBC::ODBC"
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
    "${PROJECT_SOURCE_DIR}/include/soci/odbc/soci-odbc.h"
  MISSING_DEPENDENCY_BEHAVIOR "${DEPENDENCY_MODE}"
)

if (NOT SOCI_ODBC)
  return()
endif()
