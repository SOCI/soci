if (SOCI_POSTGRESQL_AS_AVAILABLE)
  set(REQUIRED_FLAG "")
else()
  set(REQUIRED_FLAG "REQUIRED")
endif()

find_package(PostgreSQL ${REQUIRED_FLAG})

if (NOT PostgreSQL_FOUND)
  message(STATUS "Disabling PostgreSQL backend as the required dependencies were not found")
  set(SOCI_POSTGRESQL OFF CACHE BOOL "" FORCE)
  return()
endif()

option(SOCI_POSTGRESQL_NO_LO64
  "Do not use lo_xxx64() functions for compatibility with PostgreSQL < 9.3"
  OFF
)
if (POSTGRESQL_VERSION VERSION_LESS "9.3.0")
  set(SOCI_POSTGRESQL_NO_LO64 ON CACHE BOOL "Avoid using lo_xxx64() functions" FORCE)
endif()

add_library(soci_postgresql
  ${SOCI_LIB_TYPE}
    "blob.cpp"
    "error.cpp"
    "factory.cpp"
    "row-id.cpp"
    "session.cpp"
    "standard-into-type.cpp"
    "standard-use-type.cpp"
    "statement.cpp"
    "vector-into-type.cpp"
    "vector-use-type.cpp"
)

add_library(SOCI::PostgreSQL ALIAS soci_postgresql)

set_target_properties(
  soci_postgresql PROPERTIES
  SOVERSION ${PROJECT_VERSION_MAJOR}
  VERSION ${PROJECT_VERSION}
  EXPORT_NAME PostgreSQL
)

soci_public_dependency(
  NAME PostgreSQL
  DEP_TARGETS PostgreSQL::PostgreSQL
  TARGET SOCI::PostgreSQL
  REQUIRED
)

soci_public_dependency(
  NAME SOCI::Core
  DEP_TARGETS SOCI::Core
  TARGET SOCI::PostgreSQL
  REQUIRED
)

target_include_directories(soci_postgresql
  PRIVATE
    "${PROJECT_SOURCE_DIR}/include/private"
    "${PROJECT_SOURCE_DIR}/include/soci"
)

target_compile_definitions(soci_postgresql INTERFACE SOCI_POSTGRESQL_NO_LO64)

target_link_libraries(soci_interface INTERFACE SOCI::PostgreSQL)


install(
  TARGETS soci_postgresql
  EXPORT SOCIPostgreSQLTargets
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}/soci"
    COMPONENT soci_runtime
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/soci"
    COMPONENT          soci_runtime
    NAMELINK_COMPONENT soci_development
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}/soci"
    COMPONENT soci_development
  FILE_SET headers DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/soci"
    COMPONENT soci_development
)
# Generate and install a targets file
install(
  EXPORT SOCIPostgreSQLTargets
  DESTINATION "${SOCI_INSTALL_CMAKEDIR}"
  FILE SOCIPostgreSQLTargets.cmake
  NAMESPACE SOCI::
  COMPONENT soci_development
)
