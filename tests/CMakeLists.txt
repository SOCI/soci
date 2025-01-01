
add_subdirectory(common)


foreach (CURRENT_BACKEND IN ITEMS "db2" "empty" "firebird" "mysql" "odbc" "oracle" "postgresql" "sqlite3")
  string(TOUPPER "${CURRENT_BACKEND}" CURRENT_BACKEND_UPPER)

  if (SOCI_${CURRENT_BACKEND_UPPER} AND NOT SOCI_${CURRENT_BACKEND_UPPER}_DO_NOT_TEST)
    add_subdirectory(${CURRENT_BACKEND})
  endif()
endforeach()

