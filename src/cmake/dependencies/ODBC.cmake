set(ODBC_FIND_QUIETLY TRUE)

find_package(ODBC)

boost_external_report(ODBC INCLUDE_DIRECTORIES LIBRARIES)

#if(MYSQL_FOUND)
#  include_directories(${MYSQL_INCLUDE_DIR})
#  add_definitions(-DHAVE_MYSQL)
#endif()