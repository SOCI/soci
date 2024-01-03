if (SOCI_DB2_AS_AVAILABLE)
  set(REQUIRED_FLAG "")
else()
  set(REQUIRED_FLAG "REQUIRED")
endif()

find_package(DB2 ${REQUIRED_FLAG})

if (NOT DB2_FOUND)
  message(STATUS "Disabling DB2 backend as the required dependencies were not found")
  return()
endif()

add_library(soci_db2_objects OBJECT
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

target_link_libraries(soci_db2_objects PUBLIC DB2::db2)
target_link_libraries(soci_db2_objects PUBLIC soci_core_objects)

target_include_directories(soci_db2_objects
  PRIVATE
    "${PROJECT_SOURCE_DIR}/include/private"
    "${PROJECT_SOURCE_DIR}/include/soci"
)

if (SOCI_SHARED)
  add_library(soci_db2)
  target_link_libraries(soci_db2 PUBLIC soci_db2_objects)
  add_library(SOCI::shared::DB2 ALIAS soci_db2)
endif()
if (SOCI_STATIC)
  add_library(soci_db2_static)
  target_link_libraries(soci_db2_static PUBLIC soci_db2_objects)
  add_library(SOCI::static::DB2 ALIAS soci_db2_static)
endif()
