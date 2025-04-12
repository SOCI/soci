include(soci_define_backend_target)

if (SOCI_POSTGRESQL_AUTO)
  set(DEPENDENCY_MODE "DISABLE")
else()
  set(DEPENDENCY_MODE "ERROR")
endif()

soci_define_backend_target(
  NAME "PostgreSQL"
  TARGET_NAME "soci_postgresql"
  DEPENDENCIES
    "PostgreSQL YIELDS PostgreSQL::PostgreSQL"
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
    "${PROJECT_SOURCE_DIR}/include/soci/postgresql/soci-postgresql.h"
  MISSING_DEPENDENCY_BEHAVIOR "${DEPENDENCY_MODE}"
)

if (NOT SOCI_POSTGRESQL)
  return()
endif()

option(SOCI_POSTGRESQL_NO_LO64
  "Do not use lo_xxx64() functions for compatibility with PostgreSQL < 9.3"
  OFF
)
if (POSTGRESQL_VERSION VERSION_LESS "9.3.0")
  set(SOCI_POSTGRESQL_NO_LO64 ON CACHE BOOL "Avoid using lo_xxx64() functions" FORCE)
endif()

if (SOCI_POSTGRESQL_NO_LO64)
  target_compile_definitions(soci_postgresql INTERFACE SOCI_POSTGRESQL_NO_LO64)
endif()
