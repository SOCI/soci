if (SOCI_SQLITE3_AUTO)
  set(REQUIRED_FLAG "")
else()
  set(REQUIRED_FLAG "REQUIRED")
endif()

find_package(SQLite3 ${REQUIRED_FLAG})

if (NOT SQLite3_FOUND)
  message(STATUS "Disabling SQLite3 backend as the required dependencies were not found")
  set(SOCI_SQLITE3 OFF CACHE BOOL "" FORCE)
  return()
endif()

add_library(soci_sqlite3
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

add_library(SOCI::SQLite3 ALIAS soci_sqlite3)

set_target_properties(
  soci_sqlite3 PROPERTIES
  SOVERSION ${PROJECT_VERSION_MAJOR}
  VERSION ${PROJECT_VERSION}
  EXPORT_NAME SQLite3
)

target_sources(soci_sqlite3
  PUBLIC
    FILE_SET headers TYPE HEADERS
    BASE_DIRS "${PROJECT_SOURCE_DIR}/include"
    FILES
      "${PROJECT_SOURCE_DIR}/include/soci/sqlite3/soci-sqlite3.h"
)

soci_public_dependency(
  NAME SQLite3
  DEP_TARGETS SQLite::SQLite3
  TARGET SOCI::SQLite3
  REQUIRED
)
soci_public_dependency(
  NAME SOCI::Core
  DEP_TARGETS SOCI::Core
  TARGET SOCI::SQLite3
  REQUIRED
)

target_include_directories(soci_sqlite3
  PRIVATE
    "${PROJECT_SOURCE_DIR}/include/private"
    "${PROJECT_SOURCE_DIR}/include/soci"
)

target_link_libraries(soci_interface INTERFACE SOCI::SQLite3)


install(
  TARGETS soci_sqlite3
  EXPORT SOCISQLite3Targets
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
  EXPORT SOCISQLite3Targets
  DESTINATION "${SOCI_INSTALL_CMAKEDIR}"
  FILE SOCISQLite3Targets.cmake
  NAMESPACE SOCI::
  COMPONENT soci_development
)
