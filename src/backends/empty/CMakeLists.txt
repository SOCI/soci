add_library(soci_empty_interface INTERFACE)
target_link_libraries(soci_empty_interface INTERFACE soci_core_interface)

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

target_link_libraries(soci_empty_objects PUBLIC soci_empty_interface)

target_include_directories(soci_empty_objects
  PRIVATE
    "${PROJECT_SOURCE_DIR}/include/private"
    "${PROJECT_SOURCE_DIR}/include/soci"
)

soci_backend_objects_to_lib(
  OBJECT_LIB         soci_empty_objects
  SHARED_TARGET_NAME soci_empty
  STATIC_TARGET_NAME soci_empty_static
  ALIAS_NAME         Empty
  SHARED_DEPS        SOCI::shared::Core
  STATIC_DEPS        SOCI::static::Core
)
