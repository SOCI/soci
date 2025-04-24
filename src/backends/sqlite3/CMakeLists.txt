include(soci_define_backend_target)

if (SOCI_SQLITE3_AUTO)
  # Unlike most other backends, we can fall back on using the built-in
  # version of SQLite3 if the system version is not found.
  set(DEPENDENCY_MODE "BUILTIN")
else()
  set(DEPENDENCY_MODE "ERROR")
endif()

# This variable is specific to this backend and allows to either force or
# disable using the built-in copy of SQLite3.
set(SOCI_SQLITE3_BUILTIN "" CACHE STRING "Prefer, or forbid, using the built-in SQLite3 library")

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
  set(SQLITE3_DIRECTORY "${PROJECT_SOURCE_DIR}/3rdparty/sqlite3")
  set(SQLITE3_SOURCE_FILE "${SQLITE3_DIRECTORY}/sqlite3.c")

  # But if it wasn't, use our built-in version, after checking that it is
  # available, and if we're allowed to use it.
  if (NOT "${SOCI_SQLITE3_BUILTIN}" STREQUAL "OFF")
    if (NOT EXISTS ${SQLITE3_SOURCE_FILE})
      message(WARNING "SQLite3 not found and built-in version not available, have you cloned SOCI repository with --recurse-submodules?")
    else()
      set(SOCI_SQLITE3_USE_BUILTIN ON)
    endif()
  endif()

  if (NOT SOCI_SQLITE3_USE_BUILTIN)
    get_property(DESCRIPTION CACHE SOCI_SQLITE3 PROPERTY HELPSTRING)
    set(SOCI_SQLITE3 OFF CACHE STRING "${DESCRIPTION}" FORCE)
    return()
  endif()

  if ("${SOCI_SQLITE3_BUILTIN}" STREQUAL "ON")
    message(STATUS "Using built-in version of SQLite3")
  else()
    message(STATUS "Falling back on built-in version of SQLite3")
  endif()

  target_sources(soci_sqlite3
    PRIVATE
      ${SQLITE3_SOURCE_FILE}
  )
  target_include_directories(soci_sqlite3
    PRIVATE
      ${SQLITE3_DIRECTORY}
  )
endif()
