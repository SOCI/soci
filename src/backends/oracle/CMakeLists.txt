include(soci_define_backend_target)

if (SOCI_ORACLE_AUTO)
  set(DEPENDENCY_MODE "DISABLE")
else()
  set(DEPENDENCY_MODE "ERROR")
endif()

soci_define_backend_target(
  NAME "Oracle"
  TARGET_NAME "soci_oracle"
  DEPENDENCIES
    "Oracle YIELDS Oracle::Oracle"
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
    "${PROJECT_SOURCE_DIR}/include/soci/oracle/soci-oracle.h"
  FIND_PACKAGE_FILES
    "${PROJECT_SOURCE_DIR}/cmake/find_modules/FindOracle.cmake"
  MISSING_DEPENDENCY_BEHAVIOR "${DEPENDENCY_MODE}"
)

if (NOT SOCI_ORACLE)
  return()
endif()
