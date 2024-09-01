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

add_library(SOCI::MySQL ALIAS soci_mysql)

set_target_properties(
  soci_mysql PROPERTIES
  SOVERSION ${PROJECT_VERSION_MAJOR}
  VERSION ${PROJECT_VERSION}
  EXPORT_NAME MySQL
)

target_sources(soci_mysql
  PUBLIC
    FILE_SET headers TYPE HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/include"
    FILES
      "${PROJECT_SOURCE_DIR}/include/soci/mysql/soci-mysql.h"
)

soci_public_dependency(
  NAME MySQL
  DEP_TARGETS MySQL::MySQL
  TARGET SOCI::MySQL
  REQUIRED
)

soci_public_dependency(
  NAME SOCI::Core
  DEP_TARGETS SOCI::Core
  TARGET SOCI::MySQL
  REQUIRED
)

target_include_directories(soci_mysql
  PRIVATE
    "${PROJECT_SOURCE_DIR}/include/private"
    "${PROJECT_SOURCE_DIR}/include/soci"
)

target_link_libraries(soci_interface INTERFACE SOCI::MySQL)


install(
  TARGETS soci_mysql
  EXPORT SOCIMySQLTargets
  RUNTIME DESTINATION "${SOCI_INSTALL_BINDIR}"
    COMPONENT soci_runtime
  LIBRARY DESTINATION "${SOCI_INSTALL_LIBDIR}"
    COMPONENT          soci_runtime
    NAMELINK_COMPONENT soci_development
  ARCHIVE DESTINATION "${SOCI_INSTALL_LIBDIR}"
    COMPONENT soci_development
    FILE_SET headers DESTINATION "${SOCI_INSTALL_INCLUDEDIR}"
    COMPONENT soci_development
)
# Generate and install a targets file
install(
  EXPORT SOCIMySQLTargets
  DESTINATION "${SOCI_INSTALL_CMAKEDIR}"
  FILE SOCIMySQLTargets.cmake
  NAMESPACE SOCI::
  COMPONENT soci_development
)
