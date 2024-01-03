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
  return()
endif()

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

target_link_libraries(soci_firebird_objects PUBLIC Firebird::Firebird)
target_link_libraries(soci_firebird_objects PUBLIC soci_core_objects)

target_include_directories(soci_firebird_objects
  PRIVATE
    "${PROJECT_SOURCE_DIR}/include/private"
    "${PROJECT_SOURCE_DIR}/include/soci"
)

if (SOCI_SHARED)
  add_library(soci_firebird)
  target_link_libraries(soci_firebird PUBLIC soci_firebird_objects)
  add_library(SOCI::shared::Firebird ALIAS soci_firebird)
endif()
if (SOCI_STATIC)
  add_library(soci_firebird_static)
  target_link_libraries(soci_firebird_static PUBLIC soci_firebird_objects)
  add_library(SOCI::static::Firebird ALIAS soci_firebird_static)
endif()
