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
endif()
if (SOCI_STATIC)
  add_library(soci_postgresql_static)
  target_link_libraries(soci_postgresql_static PUBLIC soci_postgresql_objects)
  add_library(SOCI::static::PostgreSQL ALIAS soci_postgresql_static)
endif()
