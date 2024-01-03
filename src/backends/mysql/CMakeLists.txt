if (SOCI_MYSQL_AS_AVAILABLE)
  set(REQUIRED_FLAG "")
else()
  set(REQUIRED_FLAG "REQUIRED")
endif()

find_package(MySQL ${REQUIRED_FLAG})

if (NOT MySQL_FOUND)
  message(STATUS "Disabling MySQL backend as the required dependencies were not found")
  return()
endif()

add_library(soci_mysql_objects OBJECT
  "blob.cpp"
  "common.cpp"
  "factory.cpp"
  "row-id.cpp"
  "session.cpp"
  "standard-into-type.cpp"
  "standard-use-type.cpp"
  "statement.cpp"
  "vector-into-type.cpp"
  "vector-use-type.cpp"
)

target_link_libraries(soci_mysql_objects PUBLIC MySQL::MySQL)
target_link_libraries(soci_mysql_objects PUBLIC soci_core_objects)

target_include_directories(soci_mysql_objects
  PRIVATE
    "${PROJECT_SOURCE_DIR}/include/private"
    "${PROJECT_SOURCE_DIR}/include/soci"
)

if (SOCI_SHARED)
  add_library(soci_mysql)
  target_link_libraries(soci_mysql PUBLIC soci_mysql_objects)
  add_library(SOCI::shared::MySQL ALIAS soci_mysql)
endif()
if (SOCI_STATIC)
  add_library(soci_mysql_static)
  target_link_libraries(soci_mysql_static PUBLIC soci_mysql_objects)
  add_library(SOCI::static::MySQL ALIAS soci_mysql_static)
endif()
