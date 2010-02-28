set(PostgreSQL_FIND_QUIETLY TRUE)

find_package(PostgreSQL)

boost_external_report(PostgreSQL INCLUDE_DIR LIBRARIES)

#if(POSTGRESQL_FOUND)
#include_directories(${POSTGRESQL_INCLUDE_DIR})
#  add_definitions(-DHAVE_POSTGRESQL)
#endif()
