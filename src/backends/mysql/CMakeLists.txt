if (SOCI_MYSQL_AS_AVAILABLE)
  set(REQUIRED_FLAG "")
else()
  set(REQUIRED_FLAG "REQUIRED")
endif()

find_package(MySQL ${REQUIRED_FLAG})

if (NOT MySQL_FOUND)
  message(STATUS "Disabling MySQL backend as the required dependencies were not found")
  set(SOCI_MYSQL OFF CACHE BOOL "" FORCE)
  return()
endif()

add_library(soci_mysql_interface INTERFACE)
target_link_libraries(soci_mysql_interface INTERFACE MySQL::MySQL soci_core_interface)

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

target_link_libraries(soci_mysql_objects PUBLIC soci_mysql_interface)

target_include_directories(soci_mysql_objects
  PRIVATE
    "${PROJECT_SOURCE_DIR}/include/private"
    "${PROJECT_SOURCE_DIR}/include/soci"
)

soci_backend_objects_to_lib(
  OBJECT_LIB         soci_mysql_objects
  SHARED_TARGET_NAME soci_mysql
  STATIC_TARGET_NAME soci_mysql_static
  ALIAS_NAME         MySQL
  SHARED_DEPS        SOCI::shared::core
  STATIC_DEPS        SOCI::static::core
)
