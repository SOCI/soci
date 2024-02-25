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

add_library(soci_mysql
  ${SOCI_LIB_TYPE}
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

target_link_libraries(soci_mysql
  PUBLIC
    MySQL::MySQL
    SOCI::Core
)

target_include_directories(soci_mysql
  PRIVATE
    "${PROJECT_SOURCE_DIR}/include/private"
    "${PROJECT_SOURCE_DIR}/include/soci"
)

add_library(SOCI::MySQL ALIAS soci_mysql)

target_link_libraries(soci_interface INTERFACE SOCI::MySQL)
