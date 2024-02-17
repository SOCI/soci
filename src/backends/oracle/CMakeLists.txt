if (SOCI_ORACLE_AS_AVAILABLE)
  set(REQUIRED_FLAG "")
else()
  set(REQUIRED_FLAG "REQUIRED")
endif()

find_package(Oracle ${REQUIRED_FLAG})

if (NOT Oracle_FOUND)
  message(STATUS "Disabling Oracle backend as the required dependencies were not found")
  set(SOCI_ORACLE OFF CACHE BOOL "" FORCE)
  return()
endif()

add_library(soci_oracle_interface INTERFACE)
target_link_libraries(soci_oracle_interface INTERFACE Oracle::Oracle soci_core_interface)

add_library(soci_oracle_objects OBJECT
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

target_link_libraries(soci_oracle_objects PUBLIC soci_oracle_interface)

target_include_directories(soci_oracle_objects
  PRIVATE
    "${PROJECT_SOURCE_DIR}/include/private"
    "${PROJECT_SOURCE_DIR}/include/soci"
)

soci_backend_objects_to_lib(
  OBJECT_LIB         soci_oracle_objects
  SHARED_TARGET_NAME soci_oracle
  STATIC_TARGET_NAME soci_oracle_static
  ALIAS_NAME         Oracle
  SHARED_DEPS        SOCI::shared::Core
  STATIC_DEPS        SOCI::static::Core
)
