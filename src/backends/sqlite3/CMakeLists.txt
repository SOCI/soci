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

add_library(soci_sqlite3
  ${SOCI_LIB_TYPE}
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

target_link_libraries(soci_sqlite3
  PUBLIC
    SQLite::SQLite3 SOCI::Core
)

target_include_directories(soci_sqlite3
  PRIVATE
    "${PROJECT_SOURCE_DIR}/include/private"
    "${PROJECT_SOURCE_DIR}/include/soci"
)

add_library(SOCI::SQLite3 ALIAS soci_sqlite3)

target_link_libraries(soci_interface INTERFACE SOCI::SQLite3)
