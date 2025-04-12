include(soci_define_backend_target)

option(SOCI_FIREBIRD_EMBEDDED "Use the embedded version of Firebird" OFF)
if (SOCI_FIREBIRD_EMBEDDED)
  set(Firebird_SEARCH_EMBEDDED ON)
endif()

if (SOCI_FIREBIRD_AUTO)
  set(DEPENDENCY_MODE "DISABLE")
else()
  set(DEPENDENCY_MODE "ERROR")
endif()

soci_define_backend_target(
  NAME "Firebird"
  TARGET_NAME "soci_firebird"
  DEPENDENCIES
  "Firebird YIELDS Firebird::Firebird"
  SOURCE_FILES
    "blob.cpp"
    "common.cpp"
    "error-firebird.cpp"
    "factory.cpp"
    "session.cpp"
    "standard-into-type.cpp"
    "standard-use-type.cpp"
    "statement.cpp"
    "vector-into-type.cpp"
    "vector-use-type.cpp"
  HEADER_FILES
    "${PROJECT_SOURCE_DIR}/include/soci/firebird/soci-firebird.h"
  FIND_PACKAGE_FILES
    "${PROJECT_SOURCE_DIR}/cmake/find_modules/FindFirebird.cmake"
  MISSING_DEPENDENCY_BEHAVIOR "${DEPENDENCY_MODE}"
)

if (NOT SOCI_FIREBIRD)
  return()
endif()

