if (SOCI_ORACLE_AS_AVAILABLE)
  set(REQUIRED_FLAG "")
else()
  set(REQUIRED_FLAG "REQUIRED")
endif()

find_package(Oracle ${REQUIRED_FLAG})

if (NOT Oracle_FOUND)
  message(STATUS "Disabling Oracle backend as the required dependencies were not found")
  set(SOCI_ORACLE OFF CACHE BOOL "" FORCE)
  return()
endif()

add_library(soci_oracle
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
target_link_libraries(soci_oracle
  PUBLIC
    Oracle::Oracle
    SOCI::Core
)

target_include_directories(soci_oracle
  PRIVATE
    "${PROJECT_SOURCE_DIR}/include/private"
    "${PROJECT_SOURCE_DIR}/include/soci"
)

add_library(SOCI::Oracle ALIAS soci_oracle)

target_link_libraries(soci_interface INTERFACE SOCI::Oracle)
