if (SOCI_DB2_AS_AVAILABLE)
  set(REQUIRED_FLAG "")
else()
  set(REQUIRED_FLAG "REQUIRED")
endif()

find_package(DB2 ${REQUIRED_FLAG})

if (NOT DB2_FOUND)
  message(STATUS "Disabling DB2 backend as the required dependencies were not found")
  set(SOCI_DB2 OFF CACHE BOOL "" FORCE)
  return()
endif()

add_library(soci_db2
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

add_library(SOCI::DB2 ALIAS soci_db2)

set_target_properties(
  soci_db2 PROPERTIES
  SOVERSION ${PROJECT_VERSION_MAJOR}
  VERSION ${PROJECT_VERSION}
  EXPORT_NAME DB2
)

soci_public_dependency(
  NAME DB2
  DEP_TARGETS DB2::DB2
  TARGET SOCI::DB2
  REQUIRED
)
soci_public_dependency(
  NAME SOCI::Core
  DEP_TARGETS SOCI::Core
  TARGET SOCI::DB2
  REQUIRED
)

target_include_directories(soci_db2
  PRIVATE
    "${PROJECT_SOURCE_DIR}/include/private"
    "${PROJECT_SOURCE_DIR}/include/soci"
)

target_link_libraries(soci_interface INTERFACE SOCI::DB2)


install(
  TARGETS soci_db2
  EXPORT SOCIDB2Targets
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
  EXPORT SOCIDB2Targets
  DESTINATION "${SOCI_INSTALL_CMAKEDIR}"
  FILE SOCIDB2Targets.cmake
  NAMESPACE SOCI::
  COMPONENT soci_development
)
