set(SQLITE3_FIND_QUIETLY TRUE)

find_package(SQLite3)

boost_external_report(SQLite3 INCLUDE_DIR LIBRARIES)

if(SQLITE3_FOUND)
  include_directories(${SQLITE3_INCLUDE_DIR})
  add_definitions(-DHAVE_SQLITE3)
endif()
