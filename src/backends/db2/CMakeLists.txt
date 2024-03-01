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

target_link_libraries(soci_db2
  PUBLIC
    DB2::DB2
    SOCI::Core
)

target_include_directories(soci_db2
  PRIVATE
    "${PROJECT_SOURCE_DIR}/include/private"
    "${PROJECT_SOURCE_DIR}/include/soci"
)

add_library(SOCI::DB2 ALIAS soci_db2)

target_link_libraries(soci_interface INTERFACE SOCI::DB2)
