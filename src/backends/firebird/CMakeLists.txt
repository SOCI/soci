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

target_link_libraries(soci_firebird
  PUBLIC
    Firebird::Firebird
    SOCI::Core
)

target_include_directories(soci_firebird
  PRIVATE
    "${PROJECT_SOURCE_DIR}/include/private"
    "${PROJECT_SOURCE_DIR}/include/soci"
)

add_library(SOCI::Firebird ALIAS soci_firebird)

target_link_libraries(soci_interface INTERFACE SOCI::Firebird)
