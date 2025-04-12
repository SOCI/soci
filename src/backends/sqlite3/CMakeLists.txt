include(soci_define_backend_target)

if (SOCI_SQLITE3_AUTO)
  # Unlike most other backends, we can fall back on using the built-in
  # version of SQLite3 if the system version is not found.
  set(DEPENDENCY_MODE "BUILTIN")
else()
  set(DEPENDENCY_MODE "ERROR")
endif()

soci_define_backend_target(
  NAME "SQLite3"
  TARGET_NAME "soci_sqlite3"
  DEPENDENCIES
    "SQLite3 YIELDS SQLite::SQLite3"
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
    "${PROJECT_SOURCE_DIR}/include/soci/sqlite3/soci-sqlite3.h"
  MISSING_DEPENDENCY_BEHAVIOR "${DEPENDENCY_MODE}"
)

if (NOT SOCI_SQLITE3)
  return()
endif()

# If SQLite3 was found, this target must have been created.
if (NOT TARGET SQLite::SQLite3)
  # But if it wasn't, use our built-in version, after checking that it is
  # available.
  if (NOT EXISTS "${PROJECT_SOURCE_DIR}/3rdparty/sqlite3/sqlite3.c")
    message(WARNING "SQLite3 not found and built-in version not available, have you cloned SOCI repository with --recurse-submodules?")

    get_property(DESCRIPTION CACHE SOCI_SQLITE3 PROPERTY HELPSTRING)
    set(SOCI_SQLITE3 OFF CACHE STRING "${DESCRIPTION}" FORCE)
    return()
  endif()

  add_library(soci_sqlite3_builtin)
  target_sources(soci_sqlite3_builtin
    PRIVATE
      "${PROJECT_SOURCE_DIR}/3rdparty/sqlite3/sqlite3.c"
  )
  target_include_directories(soci_sqlite3_builtin
    PUBLIC
      "${PROJECT_SOURCE_DIR}/3rdparty/sqlite3"
  )

  target_link_libraries(soci_sqlite3
    PRIVATE
      soci_sqlite3_builtin
  )
endif()
