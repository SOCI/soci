include(soci_define_backend_target)

if (SOCI_DB2_AUTO)
  set(DEPENDENCY_MODE "DISABLE")
else()
  set(DEPENDENCY_MODE "ERROR")
endif()

soci_define_backend_target(
  NAME "DB2"
  TARGET_NAME "soci_db2"
  DEPENDENCIES
    "DB2 YIELDS DB2::DB2"
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
    "${PROJECT_SOURCE_DIR}/include/soci/db2/soci-db2.h"
  FIND_PACKAGE_FILES
    "${PROJECT_SOURCE_DIR}/cmake/find_modules/FindDB2.cmake"
  MISSING_DEPENDENCY_BEHAVIOR "${DEPENDENCY_MODE}"
)

if (NOT SOCI_DB2)
  return()
endif()

