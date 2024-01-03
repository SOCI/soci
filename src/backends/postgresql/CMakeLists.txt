if (SOCI_POSTGRESQL_AS_AVAILABLE)
  set(REQUIRED_FLAG "")
else()
  set(REQUIRED_FLAG "REQUIRED")
endif()

find_package(PostgreSQL ${REQUIRED_FLAG})

if (NOT PostgreSQL_FOUND)
  message(STATUS "Disabling PostgreSQL backend as the required dependencies were not found")
  return()
endif()

option(SOCI_POSTGRESQL_NO_LO64
  "Do not use lo_xxx64() functions for compatibility with PostgreSQL < 9.3"
  OFF
)
if (POSTGRESQL_VERSION VERSION_LESS "9.3.0")
  set(SOCI_POSTGRESQL_NO_LO64 ON CACHE BOOL "Avoid using lo_xxx64() functions" FORCE)
endif()

add_library(soci_postgresql_objects OBJECT
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
)

target_link_libraries(soci_postgresql_objects PUBLIC PostgreSQL::PostgreSQL)
target_link_libraries(soci_postgresql_objects PUBLIC soci_core_objects)

target_include_directories(soci_postgresql_objects
  PRIVATE
    "${PROJECT_SOURCE_DIR}/include/private"
    "${PROJECT_SOURCE_DIR}/include/soci"
)

if (SOCI_SHARED)
  add_library(soci_postgresql)
  target_link_libraries(soci_postgresql PUBLIC soci_postgresql_objects)
  add_library(SOCI::shared::PostgreSQL ALIAS soci_postgresql)
  if (SOCI_POSTGRESQL_NO_LO64)
    target_compile_definitions(soci_postgresql INTERFACE SOCI_POSTGRESQL_NO_LO64)
  endif()
endif()
if (SOCI_STATIC)
  add_library(soci_postgresql_static)
  target_link_libraries(soci_postgresql_static PUBLIC soci_postgresql_objects)
  add_library(SOCI::static::PostgreSQL ALIAS soci_postgresql_static)
  if (SOCI_POSTGRESQL_NO_LO64)
    target_compile_definitions(soci_postgresql_static INTERFACE SOCI_POSTGRESQL_NO_LO64)
  endif()
endif()
