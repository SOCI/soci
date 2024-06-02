add_library(soci_common_tests STATIC common-tests.cpp)
target_include_directories(soci_common_tests
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(soci_common_tests PUBLIC SOCI::Core)
target_include_directories(soci_common_tests PUBLIC "${PROJECT_SOURCE_DIR}/include/private/")

foreach (CURRENT_BACKEND IN ITEMS "db2" "empty" "firebird" "mysql" "odbc" "oracle" "postgresql" "sqlite3")
  string(TOUPPER "${CURRENT_BACKEND}" CURRENT_BACKEND_UPPER)

  if (SOCI_${CURRENT_BACKEND_UPPER} AND NOT SOCI_${CURRENT_BACKEND_UPPER}_DO_NOT_TEST)
    add_subdirectory(${CURRENT_BACKEND})
  endif()
endforeach()
