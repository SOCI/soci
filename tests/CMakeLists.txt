include(soci_utils)

add_library(soci_common_tests STATIC common-tests.cpp)
target_include_directories(soci_common_tests
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(soci_common_tests PUBLIC soci_core_interface)
target_include_directories(soci_common_tests PUBLIC "${PROJECT_SOURCE_DIR}/include/private/")


function(soci_make_tests)
  set(FLAGS "")
  set(ONE_VAL_OPTIONS "CONNECTION_STRING" "OBJECT_LIB" "SOCI_DEP_ALIAS" "SHARED_NAME" "STATIC_NAME")
  set(MULTI_VAL_OPTIONS "")
  cmake_parse_arguments(SOCI_MAKE_TESTS "${FLAGS}" "${ONE_VAL_OPTIONS}" "${MULTI_VAL_OPTIONS}" ${ARGV})
  soci_verify_parsed_arguments(
    PREFIX "SOCI_MAKE_TESTS"
    FUNCTION_NAME "soci_make_tests"
    REQUIRED "CONNECTION_STRING" "OBJECT_LIB" "SHARED_NAME" "STATIC_NAME"
  )

  set(CREATED_TESTS "")

  if (SOCI_SHARED)
    add_executable(${SOCI_MAKE_TESTS_SHARED_NAME} "${SOCI_CXX_DUMMY_SOURCE}")
    target_link_libraries(${SOCI_MAKE_TESTS_SHARED_NAME} PRIVATE ${SOCI_MAKE_TESTS_OBJECT_LIB})
    if (SOCI_MAKE_TESTS_SOCI_DEP_ALIAS)
      target_link_libraries(${SOCI_MAKE_TESTS_SHARED_NAME} PRIVATE SOCI::shared::${SOCI_MAKE_TESTS_SOCI_DEP_ALIAS})
    endif()

    list(APPEND CREATED_TESTS "${SOCI_MAKE_TESTS_SHARED_NAME}")
  endif()

  if (SOCI_STATIC)
    add_executable(${SOCI_MAKE_TESTS_STATIC_NAME} "${SOCI_CXX_DUMMY_SOURCE}")
    target_link_libraries(${SOCI_MAKE_TESTS_STATIC_NAME} PRIVATE ${SOCI_MAKE_TESTS_OBJECT_LIB})
    if (SOCI_MAKE_TESTS_SOCI_DEP_ALIAS)
      target_link_libraries(${SOCI_MAKE_TESTS_STATIC_NAME} PRIVATE SOCI::static::${SOCI_MAKE_TESTS_SOCI_DEP_ALIAS})
    endif()

    list(APPEND CREATED_TESTS "${SOCI_MAKE_TESTS_STATIC_NAME}")
  endif()

  foreach (CURRENT IN LISTS CREATED_TESTS)
    add_test(
      NAME "${CURRENT}"
      COMMAND "${CURRENT}" "${SOCI_MAKE_TESTS_CONNECTION_STRING}" "--invisibles"
    )
  endforeach()
endfunction()

foreach (CURRENT_BACKEND IN ITEMS "db2" "empty" "firebird" "mysql" "odbc" "oracle" "postgresql" "sqlite3")
  string(TOUPPER "${CURRENT_BACKEND}" CURRENT_BACKEND_UPPER)

  if (SOCI_${CURRENT_BACKEND_UPPER} AND NOT SOCI_${CURRENT_BACKEND_UPPER}_DO_NOT_TEST)
    add_subdirectory(${CURRENT_BACKEND})
  endif()
endforeach()
