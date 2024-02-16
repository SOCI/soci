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

add_library(soci_firebird_interface INTERFACE)
target_link_libraries(soci_firebird_interface INTERFACE Firebird::Firebird soci_core_interface)

add_library(soci_firebird_objects OBJECT
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

target_link_libraries(soci_firebird_objects PUBLIC Firebird::Firebird soci_firebird_interface)

target_include_directories(soci_firebird_objects
  PRIVATE
    "${PROJECT_SOURCE_DIR}/include/private"
    "${PROJECT_SOURCE_DIR}/include/soci"
)

soci_backend_objects_to_lib(
  OBJECT_LIB         soci_firebird_objects
  SHARED_TARGET_NAME soci_firebird
  STATIC_TARGET_NAME soci_firebird_static
  ALIAS_NAME         Firebird
  SHARED_DEPS        SOCI::shared::core
  STATIC_DEPS        SOCI::static::core
)
