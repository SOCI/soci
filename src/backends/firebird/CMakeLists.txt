option(SOCI_FIREBIRD_EMBEDDED "Use the embedded version of Firebird" OFF)

if (SOCI_FIREBIRD_AS_AVAILABLE)
  set(REQUIRED_FLAG "")
else()
  set(REQUIRED_FLAG "REQUIRED")
endif()

if (SOCI_FIREBIRD_EMBEDDED)
  set(Firebird_SEARCH_EMBEDDED ON)
endif()

find_package(Firebird ${REQUIRED_FLAG})

if (NOT Firebird_FOUND)
  message(STATUS "Disabling Firebird backend as the required dependencies were not found")
  set(SOCI_FIREBIRD OFF CACHE BOOL "" FORCE)
  return()
endif()

add_library(soci_firebird
  ${SOCI_LIB_TYPE}
    "blob.cpp"
    "common.cpp"
    "error-firebird.cpp"
    "factory.cpp"
    "session.cpp"
    "standard-into-type.cpp"
    "standard-use-type.cpp"
    "statement.cpp"
    "vector-into-type.cpp"
    "vector-use-type.cpp"
)

add_library(SOCI::Firebird ALIAS soci_firebird)

set_target_properties(
  soci_firebird PROPERTIES
  SOVERSION ${PROJECT_VERSION_MAJOR}
  VERSION ${PROJECT_VERSION}
  EXPORT_NAME Firebird
)

soci_public_dependency(
  NAME Firebird
  DEP_TARGETS Firebird::Firebird
  TARGET SOCI::Firebird
  REQUIRED
)
soci_public_dependency(
  NAME SOCI::Core
  DEP_TARGETS SOCI::Core
  TARGET SOCI::Firebird
  REQUIRED
)

target_include_directories(soci_firebird
  PRIVATE
    "${PROJECT_SOURCE_DIR}/include/private"
    "${PROJECT_SOURCE_DIR}/include/soci"
)

target_link_libraries(soci_interface INTERFACE SOCI::Firebird)


install(
  TARGETS soci_firebird
  EXPORT SOCIFirebirdTargets
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
  EXPORT SOCIFirebirdTargets
  DESTINATION "${SOCI_INSTALL_CMAKEDIR}"
  FILE SOCIFirebirdTargets.cmake
  NAMESPACE SOCI::
  COMPONENT soci_development
)
