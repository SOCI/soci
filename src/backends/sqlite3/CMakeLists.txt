if (SOCI_SQLITE3_AS_AVAILABLE)
  set(REQUIRED_FLAG "")
else()
  set(REQUIRED_FLAG "REQUIRED")
endif()

find_package(SQLite3 ${REQUIRED_FLAG})

if (NOT SQLite3_FOUND)
  message(STATUS "Disabling SQLite3 backend as the required dependencies were not found")
  return()
endif()

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

target_link_libraries(soci_sqlite3_objects PUBLIC SQLite::SQLite3)
target_link_libraries(soci_sqlite3_objects PUBLIC soci_core_objects)

target_include_directories(soci_sqlite3_objects
  PRIVATE
    "${PROJECT_SOURCE_DIR}/include/private"
    "${PROJECT_SOURCE_DIR}/include/soci"
)

if (SOCI_SHARED)
  add_library(soci_sqlite3)
  target_link_libraries(soci_sqlite3 PUBLIC soci_sqlite3_objects)
  add_library(SOCI::shared::SQLite3 ALIAS soci_sqlite3)
endif()
if (SOCI_STATIC)
  add_library(soci_sqlite3_static)
  target_link_libraries(soci_sqlite3_static PUBLIC soci_sqlite3_objects)
  add_library(SOCI::static::SQLite3 ALIAS soci_sqlite3_static)
endif()
