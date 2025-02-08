# Take care of defining ALIAS targets to keep old target names working

if (SOCI_SHARED)
  set(SUFFIX "")
else()
  set(SUFFIX "_static")
endif()

set(OLD_NAMES   core db2 empty firebird mysql odbc oracle postgresql sqlite3)
set(ALIAS_NAMES Core DB2 Empty Firebird MySQL ODBC Oracle PostgreSQL SQLite3)

list(LENGTH ALIAS_NAMES N_ITEMS)
math(EXPR LAST_ITEM "${N_ITEMS} - 1")

foreach (I RANGE ${LAST_ITEM})
  list(GET OLD_NAMES ${I} CURRENT_NAME)
  list(GET ALIAS_NAMES ${I} CURRENT_ALIAS)

  if (NOT TARGET SOCI::${CURRENT_ALIAS})
    continue()
  endif()

  get_target_property(UNDERLYING_LIB SOCI::${CURRENT_ALIAS} ALIASED_TARGET)

  add_library(Soci::${CURRENT_NAME}${SUFFIX} ALIAS ${UNDERLYING_LIB})
  add_library(SOCI::${CURRENT_NAME}${SUFFIX} ALIAS ${UNDERLYING_LIB})
endforeach()
