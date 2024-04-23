if (SOCI_ODBC_AS_AVAILABLE)
  set(REQUIRED_FLAG "")
else()
  set(REQUIRED_FLAG "REQUIRED")
endif()

find_package(ODBC ${REQUIRED_FLAG})

if (NOT ODBC_FOUND)
  message(STATUS "Disabling ODBC backend as the required dependencies were not found")
  set(SOCI_ODBC OFF CACHE BOOL "" FORCE)
  return()
endif()

add_library(soci_odbc
  ${SOCI_LIB_TYPE}
    "blob.cpp"
    "factory.cpp"
    "row-id.cpp"
    "session.cpp"
    "standard-into-type.cpp"
    "standard-use-type.cpp"
    "statement.cpp"
    "vector-into-type.cpp"
    "vector-use-type.cpp"
)

add_library(SOCI::ODBC ALIAS soci_odbc)

set_target_properties(
  soci_odbc PROPERTIES
  SOVERSION ${PROJECT_VERSION_MAJOR}
  VERSION ${PROJECT_VERSION}
  EXPORT_NAME ODBC
)

soci_public_dependency(
  NAME ODBC
  DEP_TARGETS ODBC::ODBC
  TARGET SOCI::ODBC
  REQUIRED
)

soci_public_dependency(
  NAME SOCI::Core
  DEP_TARGETS SOCI::Core
  TARGET SOCI::ODBC
  REQUIRED
)

target_include_directories(soci_odbc
  PRIVATE
    "${PROJECT_SOURCE_DIR}/include/private"
    "${PROJECT_SOURCE_DIR}/include/soci"
)

target_link_libraries(soci_interface INTERFACE SOCI::ODBC)


install(
  TARGETS soci_odbc
  EXPORT SOCIODBCTargets
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
  EXPORT SOCIODBCTargets
  DESTINATION "${SOCI_INSTALL_CMAKEDIR}"
  FILE SOCIODBCTargets.cmake
  NAMESPACE SOCI::
  COMPONENT soci_development
)
