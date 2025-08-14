add_library(soci_interface INTERFACE)
add_library(SOCI::SOCI ALIAS soci_interface)

if (SOCI_VERSION VERSION_LESS 5)
  add_library(soci_interface_deprecated INTERFACE)
  target_link_libraries(soci_interface_deprecated INTERFACE soci_interface)
  set_target_properties(soci_interface_deprecated PROPERTIES DEPRECATION "Use SOCI::SOCI (all-uppercase) instead of SOCI::soci")
  add_library(SOCI::soci ALIAS soci_interface_deprecated)
endif()

set(SOCI_GENERATED_INCLUDES_DIR "${CMAKE_BINARY_DIR}/include")
file(MAKE_DIRECTORY "${SOCI_GENERATED_INCLUDES_DIR}/soci")

add_subdirectory(core)
add_subdirectory(backends)


# Generate the soci-config.h file

set(CONFIG_LINES "#ifndef SOCI_SOCICONFIG_H_INCLUDED" "#define SOCI_SOCICONFIG_H_INCLUDED" "")

set(CONFIG_VARS
  SOCI_EMPTY
  SOCI_DB2
  SOCI_FIREBIRD
  SOCI_MYSQL
  SOCI_ODBC
  SOCI_ORACLE
  SOCI_POSTGRESQL
  SOCI_SQLITE3
)
set(CONFIG_MACROS
  SOCI_HAVE_EMPTY
  SOCI_HAVE_DB2
  SOCI_HAVE_FIREBIRD
  SOCI_HAVE_MYSQL
  SOCI_HAVE_ODBC
  SOCI_HAVE_ORACLE
  SOCI_HAVE_POSTGRESQL
  SOCI_HAVE_SQLITE3
)

list(LENGTH CONFIG_MACROS N_CONFIGS)
math(EXPR LAST_CONFIG_IDX "${N_CONFIGS} - 1")

foreach (I RANGE ${LAST_CONFIG_IDX})
  list(GET CONFIG_VARS ${I} CURRENT_VAR)
  list(GET CONFIG_MACROS ${I} CURRENT_MACRO)

  if (${CURRENT_VAR})
    if ("${${CURRENT_VAR}}" MATCHES "[0-9]+")
      list(APPEND CONFIG_LINES "#define ${CURRENT_MACRO} ${${CURRENT_VAR}}")
    else()
      # Assume this is a boolean flag
      list(APPEND CONFIG_LINES "#define ${CURRENT_MACRO} 1")
    endif()
  else()
    list(APPEND CONFIG_LINES "/* #define ${CURRENT_MACRO} */")
  endif()
  list(APPEND CONFIG_LINES "")
endforeach()

list(APPEND CONFIG_LINES "#endif")

list(JOIN CONFIG_LINES "\n" CONFIG_CONTENT)

# We use file(GENERATE) to only overwrite soci-config.h if the generated
# content to be written has differed from the file's content. Note that we do
# *not* use file(WRITE) because the line endings would be CRLF on Windows, so
# the file MD5 hash would *never* equal that of the content hash, as the string
# content is of course using standard LF line endings. Using file(WRITE) would
# mean that soci-config.h is updated on each configure, so each subsequent
# cmake --build would result in the needless rebuilding we wanted to avoid.
#
file(GENERATE
  OUTPUT "${SOCI_GENERATED_INCLUDES_DIR}/soci/soci-config.h"
  CONTENT "${CONFIG_CONTENT}" NEWLINE_STYLE LF
)

# Append the generated config header to the public headers of the core target
target_sources(soci_core
  PUBLIC
    FILE_SET headers TYPE HEADERS
    BASE_DIRS "${SOCI_GENERATED_INCLUDES_DIR}"
    FILES
      "${SOCI_GENERATED_INCLUDES_DIR}/soci/soci-config.h"
)
