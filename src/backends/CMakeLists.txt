include(soci_utils)

set(SOCI_EMPTY ${PROJECT_IS_TOP_LEVEL} CACHE STRING "Include the 'empty' backend. Can be bool-valued or one of 'Enabled', 'Disabled' and 'AsAvailable'")

set(SOCI_DB2 "AsAvailable" CACHE STRING "Include the 'DB2' backend. Can be bool-valued or one of 'Enabled', 'Disabled' and 'AsAvailable'")
set(SOCI_FIREBIRD "AsAvailable" CACHE STRING "Include the 'Firebird' backend. Can be bool-valued or one of 'Enabled', 'Disabled' and 'AsAvailable'")
set(SOCI_MYSQL "AsAvailable" CACHE STRING "Include the 'MySQL' backend. Can be bool-valued or one of 'Enabled', 'Disabled' and 'AsAvailable'")
set(SOCI_ODBC "AsAvailable" CACHE STRING "Include the 'ODBC' backend. Can be bool-valued or one of 'Enabled', 'Disabled' and 'AsAvailable'")
set(SOCI_ORACLE "AsAvailable" CACHE STRING "Include the 'Oracle' backend. Can be bool-valued or one of 'Enabled', 'Disabled' and 'AsAvailable'")
set(SOCI_POSTGRESQL "AsAvailable" CACHE STRING "Include the 'Postgresql' backend. Can be bool-valued or one of 'Enabled', 'Disabled' and 'AsAvailable'")
set(SOCI_SQLITE3 "AsAvailable" CACHE STRING "Include the 'SQLite3' backend. Can be bool-valued or one of 'Enabled', 'Disabled' and 'AsAvailable'")


function(soci_backend_objects_to_lib)
  set(FLAGS "")
  set(ONE_VAL_OPTIONS "OBJECT_LIB" "SHARED_TARGET_NAME" "STATIC_TARGET_NAME" "ALIAS_NAME")
  set(MULTI_VAL_OPTIONS "SHARED_DEPS" "STATIC_DEPS")
  cmake_parse_arguments(SOCI_BACKEND_LIB "${FLAGS}" "${ONE_VAL_OPTIONS}" "${MULTI_VAL_OPTIONS}" ${ARGV})
  soci_verify_parsed_arguments(
    PREFIX "SOCI_BACKEND_LIB"
    FUNCTION_NAME "soci_backend_objects_to_lib"
    REQUIRED "OBJECT_LIB" "SHARED_TARGET_NAME" "STATIC_TARGET_NAME" "ALIAS_NAME"
  )

  if (SOCI_SHARED)
    add_library(${SOCI_BACKEND_LIB_SHARED_TARGET_NAME} SHARED "${SOCI_CXX_DUMMY_SOURCE}")
    target_link_libraries(${SOCI_BACKEND_LIB_SHARED_TARGET_NAME}
      PUBLIC
        ${SOCI_BACKEND_LIB_OBJECT_LIB}
        ${SOCI_BACKEND_LIB_SHARED_DEPS}
    )

    add_library(SOCI::shared::${SOCI_BACKEND_LIB_ALIAS_NAME} ALIAS ${SOCI_BACKEND_LIB_SHARED_TARGET_NAME})

    target_link_libraries(soci_shared_interface INTERFACE SOCI::shared::${SOCI_BACKEND_LIB_ALIAS_NAME})
  endif()

  if (SOCI_STATIC)
    add_library(${SOCI_BACKEND_LIB_STATIC_TARGET_NAME} STATIC "${SOCI_CXX_DUMMY_SOURCE}")
    target_link_libraries(${SOCI_BACKEND_LIB_STATIC_TARGET_NAME}
      PUBLIC
        ${SOCI_BACKEND_LIB_OBJECT_LIB}
        ${SOCI_BACKEND_LIB_STATIC_DEPS}
    )

    add_library(SOCI::static::${SOCI_BACKEND_LIB_ALIAS_NAME} ALIAS ${SOCI_BACKEND_LIB_STATIC_TARGET_NAME})

    target_link_libraries(soci_static_interface INTERFACE SOCI::static::${SOCI_BACKEND_LIB_ALIAS_NAME})
  endif()
endfunction()


foreach(CURRENT IN ITEMS "db2" "empty" "firebird" "mysql" "odbc" "oracle" "postgresql" "sqlite3")
  string(TOUPPER "${CURRENT}" CURRENT_UPPER)

  # Backwards compatibility with the old cmake setup that used WITH_* variables
  # These are now only defined if the user specified them.
  if (DEFINED WITH_${CURRENT_UPPER})
    message(DEPRECATION "Use of the WITH_${CURRENT_UPPER} option is deprecated - use the new SOCI_${CURRENT_UPPER} instead")
    set(SOCI_${CURRENT_UPPER} "${WITH_${CURRENT_UPPER}}" CACHE STRING "")
    # Delete the legacy variable from cache
    unset(WITH_${CURRENT_UPPER} CACHE)
  endif()

  set(SOCI_${CURRENT_UPPER}_AS_AVAILABLE OFF CACHE INTERNAL "" FORCE)

  if (SOCI_${CURRENT_UPPER} STREQUAL "Enabled")
    set(SOCI_${CURRENT_UPPER} ON CACHE BOOL "" FORCE)
  elseif(SOCI_${CURRENT_UPPER} STREQUAL "Disabled")
    set(SOCI_${CURRENT_UPPER} OFF CACHE BOOL "" FORCE)
  elseif(SOCI_${CURRENT_UPPER} STREQUAL "AsAvailable")
    set(SOCI_${CURRENT_UPPER} ON CACHE BOOL "" FORCE)
    set(SOCI_${CURRENT_UPPER}_AS_AVAILABLE ON CACHE INTERNAL "" FORCE)
  elseif(SOCI_${CURRENT_UPPER} MATCHES "[Oo][Nn]|[Oo][Ff][Ff]|[Tt][Rr][Uu][Ee]|[Ff][Aa][Ll][Ss][Ee]|[Yy][Ee][Ss]|[Nn][Oo]|[YyNn01]")
    # Do nothing - these are already booleans
  else()
    message(FATAL_ERROR "Unsupported value '${SOCI_${CURRENT_UPPER}}' for option SOCI_${CURRENT_UPPER}")
  endif()

  if (SOCI_${CURRENT_UPPER})
    add_subdirectory(${CURRENT})
  endif()
endforeach()
