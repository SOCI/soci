if (SOCI_ODBC_AS_AVAILABLE)
  set(REQUIRED_FLAG "")
else()
  set(REQUIRED_FLAG "REQUIRED")
endif()

find_package(ODBC ${REQUIRED_FLAG})

if (NOT ODBC_FOUND)
  message(STATUS "Disabling ODBC backend as the required dependencies were not found")
  set(SOCI_ODBC OFF CACHE BOOL "" FORCE)
  return()
endif()

add_library(soci_odbc_interface INTERFACE)
target_link_libraries(soci_odbc_interface INTERFACE ODBC::ODBC soci_core_interface)

add_library(soci_odbc_objects OBJECT
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

target_link_libraries(soci_odbc_objects PUBLIC soci_odbc_interface)

target_include_directories(soci_odbc_objects
  PRIVATE
    "${PROJECT_SOURCE_DIR}/include/private"
    "${PROJECT_SOURCE_DIR}/include/soci"
)

soci_backend_objects_to_lib(
  OBJECT_LIB         soci_odbc_objects
  SHARED_TARGET_NAME soci_odbc
  STATIC_TARGET_NAME soci_odbc_static
  ALIAS_NAME         ODBC
  SHARED_DEPS        SOCI::shared::core
  STATIC_DEPS        SOCI::static::core
)
