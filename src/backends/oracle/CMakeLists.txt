if (SOCI_ORACLE_AUTO)
  set(REQUIRED_FLAG "")
else()
  set(REQUIRED_FLAG "REQUIRED")
endif()

find_package(Oracle ${REQUIRED_FLAG})

if (NOT Oracle_FOUND)
  message(STATUS "Disabling Oracle backend as the required dependencies were not found")
  set(SOCI_ORACLE OFF CACHE BOOL "" FORCE)
  return()
endif()

add_library(soci_oracle
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

add_library(SOCI::Oracle ALIAS soci_oracle)

set_target_properties(
  soci_oracle PROPERTIES
  SOVERSION ${PROJECT_VERSION_MAJOR}
  VERSION ${PROJECT_VERSION}
  EXPORT_NAME Oracle
)

target_sources(soci_oracle
  PUBLIC
    FILE_SET headers TYPE HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/include"
    FILES
      "${PROJECT_SOURCE_DIR}/include/soci/oracle/soci-oracle.h"
)

soci_public_dependency(
  NAME Oracle
  DEP_TARGETS Oracle::Oracle
  TARGET SOCI::Oracle
  REQUIRED
)

soci_public_dependency(
  NAME SOCI::Core
  DEP_TARGETS SOCI::Core
  TARGET SOCI::Oracle
  REQUIRED
)

target_include_directories(soci_oracle
  PRIVATE
    "${PROJECT_SOURCE_DIR}/include/private"
    "${PROJECT_SOURCE_DIR}/include/soci"
)

target_link_libraries(soci_interface INTERFACE SOCI::Oracle)


install(
  TARGETS soci_oracle
  EXPORT SOCIOracleTargets
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
  EXPORT SOCIOracleTargets
  DESTINATION "${SOCI_INSTALL_CMAKEDIR}"
  FILE SOCIOracleTargets.cmake
  NAMESPACE SOCI::
  COMPONENT soci_development
)
