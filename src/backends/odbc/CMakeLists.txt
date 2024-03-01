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

target_link_libraries(soci_odbc
  PUBLIC
    ODBC::ODBC
    SOCI::Core
)

target_include_directories(soci_odbc
  PRIVATE
    "${PROJECT_SOURCE_DIR}/include/private"
    "${PROJECT_SOURCE_DIR}/include/soci"
)

add_library(SOCI::ODBC ALIAS soci_odbc)

target_link_libraries(soci_interface INTERFACE SOCI::ODBC)
