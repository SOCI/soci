if (SOCI_SQLITE3_AS_AVAILABLE)
  set(REQUIRED_FLAG "")
else()
  set(REQUIRED_FLAG "REQUIRED")
endif()

find_package(SQLite3 ${REQUIRED_FLAG})

if (NOT SQLite3_FOUND)
  message(STATUS "Disabling SQLite3 backend as the required dependencies were not found")
  set(SOCI_SQLITE3 OFF CACHE BOOL "" FORCE)
  return()
endif()

add_library(soci_sqlite3_interface INTERFACE)
target_link_libraries(soci_sqlite3_interface INTERFACE SQLite::SQLite3 soci_core_interface)

add_library(soci_sqlite3_objects OBJECT
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

target_link_libraries(soci_sqlite3_objects PUBLIC soci_sqlite3_interface)

target_include_directories(soci_sqlite3_objects
  PRIVATE
    "${PROJECT_SOURCE_DIR}/include/private"
    "${PROJECT_SOURCE_DIR}/include/soci"
)

soci_backend_objects_to_lib(
  OBJECT_LIB         soci_sqlite3_objects
  SHARED_TARGET_NAME soci_sqlite3
  STATIC_TARGET_NAME soci_sqlite3_static
  ALIAS_NAME         SQLite3
  SHARED_DEPS        SOCI::shared::core
  STATIC_DEPS        SOCI::static::core
)
