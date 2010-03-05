# TODO: to be refactored as a macro similar to soci_backend()
#       or as actual part of soci_backend()

set(SOCI_MYSQL_TEST_TARGET test_mysql)
set(SOCI_MYSQL_TEST_SOURCES test-mysql.cpp)

include_directories(${SOCI_SOURCE_DIR}/core/test)
include_directories(${SOCI_SOURCE_DIR}/backends/mysql)

add_executable(${SOCI_MYSQL_TEST_TARGET} ${SOCI_MYSQL_TEST_SOURCES})
add_executable(${SOCI_MYSQL_TEST_TARGET}_static ${SOCI_MYSQL_TEST_SOURCES})

target_link_libraries(${SOCI_MYSQL_TEST_TARGET}
  ${SOCI_CORE_TARGET}
  ${SOCI_MYSQL_TARGET}
  ${MYSQL_LIBRARIES})

target_link_libraries(${SOCI_MYSQL_TEST_TARGET}_static
  ${SOCI_CORE_TARGET}-static
  ${SOCI_MYSQL_TARGET}-static
  ${MYSQL_LIBRARIES}
  ${SOCI_CORE_STATIC_DEPENDENCIES})

add_test(${SOCI_MYSQL_TEST_TARGET}
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${SOCI_MYSQL_TEST_TARGET})

add_test(${SOCI_MYSQL_TEST_TARGET}_static
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${SOCI_MYSQL_TEST_TARGET}_static)
