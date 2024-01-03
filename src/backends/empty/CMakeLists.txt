add_library(soci_empty_objects OBJECT
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

target_link_libraries(soci_empty_objects PUBLIC soci_core_objects)

target_include_directories(soci_empty_objects
  PRIVATE
    "${PROJECT_SOURCE_DIR}/include/private"
    "${PROJECT_SOURCE_DIR}/include/soci"
)

if (SOCI_SHARED)
  add_library(soci_empty)
  target_link_libraries(soci_empty PUBLIC soci_empty_objects)
  add_library(SOCI::shared::Empty ALIAS soci_empty)
endif()
if (SOCI_STATIC)
  add_library(soci_empty_static)
  target_link_libraries(soci_empty_static PUBLIC soci_empty_objects)
  add_library(SOCI::static::Empty ALIAS soci_empty_static)
endif()
