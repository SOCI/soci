set(SOCI_EXISTING_BACKENDS
  "DB2"
  "Empty"
  "Firebird"
  "MySQL"
  "ODBC"
  "Oracle"
  "PostgreSQL"
  "SQLite3"
  CACHE INTERNAL "" FORCE
)
set(ENABLED_BACKENDS "")

foreach(CURRENT_BACKEND IN LISTS SOCI_EXISTING_BACKENDS)
  string(TOUPPER "${CURRENT_BACKEND}" CURRENT_BACKEND_UPPER)
  string(TOLOWER "${CURRENT_BACKEND}" CURRENT_BACKEND_LOWER)

  set(CURRENT_ENABLED_VAR "SOCI_${CURRENT_BACKEND_UPPER}")
  set(
    CURRENT_DESCRIPTION
    "Whether to include the '${CURRENT_BACKEND}' backend. Can be a boolean value or 'AUTO' to include the backend if its dependencies can be satisfied and disable it otherwise."
  )

  # Remember the original value of the cache variable to be restored below.
  if (DEFINED ${CURRENT_ENABLED_VAR})
    set(ORIGINAL_VALUE "${${CURRENT_ENABLED_VAR}}")
  else()
    set(ORIGINAL_VALUE "AUTO")
  endif()

  # Add option to enable/disable the current backend. Note: if the cache variable already exists
  # (e.g. because it has been specified on the command line), this will NOT overwrite the
  # explicitly specified value
  set(${CURRENT_ENABLED_VAR} "AUTO" CACHE STRING "${CURRENT_DESCRIPTION}")


  # Backwards compatibility with the old cmake setup that used WITH_* variables
  # These are now only defined if the user specified them.
  if (DEFINED WITH_${CURRENT_BACKEND_UPPER})
    message(DEPRECATION "Use of the WITH_${CURRENT_BACKEND_UPPER} option is deprecated - use the new ${CURRENT_ENABLED_VAR} instead")
    if (WITH_${CURRENT_BACKEND_UPPER} AND ${CURRENT_ENABLED_VAR})
      # Overwrite the new-style enable-option unless that was set to disable the current backend
      # (Note that non-empty string - in particular "AUTO" - also convert to a true expression).
      # However, the legacy behavior was to only include the backend, if its dependencies are met,
      # so we set to AUTO rather than ON.
      set(${CURRENT_ENABLED_VAR} "AUTO" CACHE STRING "${CURRENT_DESCRIPTION}" FORCE)
    endif()
    # Delete the legacy variable from cache
    unset(WITH_${CURRENT_BACKEND_UPPER} CACHE)
  endif()


  # Convert the value of the current backend's include flag to uppercase
  string(TOUPPER "${${CURRENT_ENABLED_VAR}}" UPPER_VALUE)
  set(${CURRENT_ENABLED_VAR} "${UPPER_VALUE}" CACHE STRING "${CURRENT_DESCRIPTION}" FORCE)


  # Internal variable indicating whether the current backend's include mode is AUTO
  set(CURRENT_AUTO_VAR "SOCI_${CURRENT_BACKEND_UPPER}_AUTO")
  set(${CURRENT_AUTO_VAR} OFF)


  if (${CURRENT_ENABLED_VAR} STREQUAL "AUTO")
    set(${CURRENT_AUTO_VAR} ON)
  elseif (NOT ${CURRENT_ENABLED_VAR} MATCHES "^(ON|OFF|TRUE|FALSE|YES|NO|[YN01])$")
    message(FATAL_ERROR "Invalid value '${${CURRENT_ENABLED_VAR}}' for option '${CURRENT_ENABLED_VAR}'")
  endif()


  if (${CURRENT_ENABLED_VAR})
    add_subdirectory(${CURRENT_BACKEND_LOWER})

    # Verify that the backend hasn't been disabled (happens if include mode is AUTO and there are
    # unmet dependencies.
    if (${CURRENT_ENABLED_VAR})
      list(APPEND ENABLED_BACKENDS "${CURRENT_BACKEND}")
    endif()
  endif()

  # We may have modified the value of this cache variable above, but we don't
  # want to overwrite the user's value if they specified it on the command
  # line, e.g. if they set it to AUTO, it should remain set to AUTO even if we
  # didn't find the backend dependencies during this run because the user
  # might install them before rerunning CMake again. So restore the original
  # value.
  set(${CURRENT_ENABLED_VAR} "${ORIGINAL_VALUE}" CACHE STRING "${CURRENT_DESCRIPTION}" FORCE)
endforeach()

set(SOCI_ENABLED_BACKENDS "${ENABLED_BACKENDS}" CACHE INTERNAL "" FORCE)
message(STATUS "Enabled SOCI backends are: ${ENABLED_BACKENDS}")

