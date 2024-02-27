add_library(soci_interface INTERFACE)
add_library(SOCI::soci ALIAS soci_interface)

set(SOCI_GENERATED_INCLUDES_DIR "${CMAKE_BINARY_DIR}/include")
file(MAKE_DIRECTORY "${SOCI_GENERATED_INCLUDES_DIR}/soci")

add_subdirectory(core)
add_subdirectory(backends)


include(CheckCXXSourceCompiles)
check_cxx_source_compiles(
  "
      __attribute__ (( visibility(\"default\") )) int f1() { return 0; }
      __attribute__ (( visibility(\"hidden\") ))  int f2() { return 1; }

      int main(int argc, char* argv[]) { f1(); f2(); return 0; }
  "
  SOCI_VISIBILITY_ATTRIBUTE_SUPPORTED
)


# Generate the soci-config.h file

set(CONFIG_LINES "#ifndef SOCI_SOCICONFIG_H_INCLUDED" "#define SOCI_SOCICONFIG_H_INCLUDED" "")

set(CONFIG_VARS
  SOCI_HAVE_BOOST
  SOCI_HAVE_BOOST_DATE_TIME
  SOCI_EMPTY
  SOCI_DB2
  SOCI_FIREBIRD
  SOCI_MYSQL
  SOCI_ODBC
  SOCI_ORACLE
  SOCI_POSTGRESQL
  SOCI_SQLITE3
  SOCI_VISIBILITY_ATTRIBUTE_SUPPORTED
)
set(CONFIG_MACROS
  SOCI_HAVE_BOOST
  SOCI_HAVE_BOOST_DATE_TIME
  SOCI_HAVE_EMPTY
  SOCI_HAVE_DB2
  SOCI_HAVE_FIREBIRD
  SOCI_HAVE_MYSQL
  SOCI_HAVE_ODBC
  SOCI_HAVE_ORACLE
  SOCI_HAVE_POSTGRESQL
  SOCI_HAVE_SQLITE3
  SOCI_HAVE_VISIBILITY_SUPPORT
)

list(LENGTH CONFIG_MACROS N_CONFIGS)
math(EXPR LAST_CONFIG_IDX "${N_CONFIGS} - 1")

foreach (I RANGE ${LAST_CONFIG_IDX})
  list(GET CONFIG_VARS ${I} CURRENT_VAR)
  list(GET CONFIG_MACROS ${I} CURRENT_MACRO)

  if (${CURRENT_VAR})
    list(APPEND CONFIG_LINES "#define ${CURRENT_MACRO}")
  else()
    list(APPEND CONFIG_LINES "/* #define ${CURRENT_MACRO} */")
  endif()
  list(APPEND CONFIG_LINES "")
endforeach()

list(APPEND CONFIG_LINES "#endif")

list(JOIN CONFIG_LINES "\n" CONFIG_CONTENT)

string(MD5 CONFIG_CONTENT_HASH "${CONFIG_CONTENT}")
if (EXISTS "${SOCI_GENERATED_INCLUDES_DIR}/soci/soci-config.h")
  file(MD5 "${SOCI_GENERATED_INCLUDES_DIR}/soci/soci-config.h" CONFIG_FILE_HASH)
else()
  set(CONFIG_FILE_HASH 0)
endif()

# Only overwrite the soci-config.h file if the generated content is different from the
# file's content in order to avoid needless rebuilding
message(STATUS "${CONFIG_CONTENT_HASH} == ${CONFIG_FILE_HASH} ?")
if (NOT (CONFIG_CONTENT_HASH STREQUAL CONFIG_FILE_HASH))
  file(WRITE "${SOCI_GENERATED_INCLUDES_DIR}/soci/soci-config.h" "${CONFIG_CONTENT}")
endif()
