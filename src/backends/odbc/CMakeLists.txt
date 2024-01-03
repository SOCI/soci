if (SOCI_ODBC_AS_AVAILABLE)
  set(REQUIRED_FLAG "")
else()
  set(REQUIRED_FLAG "REQUIRED")
endif()

find_package(ODBC ${REQUIRED_FLAG})

if (NOT ODBC_FOUND)
  message(STATUS "Disabling ODBC backend as the required dependencies were not found")
  return()
endif()

add_library(soci_odbc_objects OBJECT
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

target_link_libraries(soci_odbc_objects PUBLIC ODBC::ODBC)
target_link_libraries(soci_odbc_objects PUBLIC soci_core_objects)

target_include_directories(soci_odbc_objects
  PRIVATE
    "${PROJECT_SOURCE_DIR}/include/private"
    "${PROJECT_SOURCE_DIR}/include/soci"
)

if (SOCI_SHARED)
  add_library(soci_odbc)
  target_link_libraries(soci_odbc PUBLIC soci_odbc_objects)
  add_library(SOCI::shared::ODBC ALIAS soci_odbc)
endif()
if (SOCI_STATIC)
  add_library(soci_odbc_static)
  target_link_libraries(soci_odbc_static PUBLIC soci_odbc_objects)
  add_library(SOCI::static::ODBC ALIAS soci_odbc_static)
endif()
