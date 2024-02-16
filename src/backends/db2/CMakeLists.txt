if (SOCI_DB2_AS_AVAILABLE)
  set(REQUIRED_FLAG "")
else()
  set(REQUIRED_FLAG "REQUIRED")
endif()

find_package(DB2 ${REQUIRED_FLAG})

if (NOT DB2_FOUND)
  message(STATUS "Disabling DB2 backend as the required dependencies were not found")
  set(SOCI_DB2 OFF CACHE BOOL "" FORCE)
  return()
endif()

add_library(soci_db2_interface INTERFACE)
target_link_libraries(soci_db2_interface INTERFACE DB2::DB2 soci_core_interface)

add_library(soci_db2_objects OBJECT
  "blob.cpp"
  "factory.cpp"
  "row-id.cpp"
  "session.cpp"
  "standard-into-type.cpp"
  "standard-use-type.cpp"
  "statement.cpp"
  "vector-into-type.cpp"
  "vector-use-type.cpp"
)

target_link_libraries(soci_db2_objects PUBLIC soci_db2_interface)

target_include_directories(soci_db2_objects
  PRIVATE
    "${PROJECT_SOURCE_DIR}/include/private"
    "${PROJECT_SOURCE_DIR}/include/soci"
)

soci_backend_objects_to_lib(
  OBJECT_LIB         soci_db2_objects
  SHARED_TARGET_NAME soci_db2
  STATIC_TARGET_NAME soci_db2_static
  ALIAS_NAME         DB2
  SHARED_DEPS        SOCI::shared::core
  STATIC_DEPS        SOCI::static::core
)
