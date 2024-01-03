if (SOCI_ORACLE_AS_AVAILABLE)
  set(REQUIRED_FLAG "")
else()
  set(REQUIRED_FLAG "REQUIRED")
endif()

find_package(Oracle ${REQUIRED_FLAG})

if (NOT Oracle_FOUND)
  message(STATUS "Disabling Oracle backend as the required dependencies were not found")
  return()
endif()

add_library(soci_oracle_objects OBJECT
  "blob.cpp"
  "clob.cpp"
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

target_link_libraries(soci_oracle_objects PUBLIC Oracle::Oracle)
target_link_libraries(soci_oracle_objects PUBLIC soci_core_objects)

target_include_directories(soci_oracle_objects
  PRIVATE
    "${PROJECT_SOURCE_DIR}/include/private"
    "${PROJECT_SOURCE_DIR}/include/soci"
)

if (SOCI_SHARED)
  add_library(soci_oracle)
  target_link_libraries(soci_oracle PUBLIC soci_oracle_objects)
  add_library(SOCI::shared::Oracle ALIAS soci_oracle)
endif()
if (SOCI_STATIC)
  add_library(soci_oracle_static)
  target_link_libraries(soci_oracle_static PUBLIC soci_oracle_objects)
  add_library(SOCI::static::Oracle ALIAS soci_oracle_static)
endif()
