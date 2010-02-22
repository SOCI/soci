set(SOCI_TEST_POSTGRESQL_TARGET test_postgresql)
set(SOCI_TEST_POSTGRESQL_SOURCES test-postgresql.cpp)

include_directories(${SOCI_SOURCE_DIR}/core/test)
include_directories(${SOCI_SOURCE_DIR}/backends/postgresql)

add_executable(${SOCI_TEST_POSTGRESQL_TARGET} ${SOCI_TEST_POSTGRESQL_SOURCES})

target_link_libraries(${SOCI_TEST_POSTGRESQL_TARGET}
  ${SOCI_CORE_TARGET}
  ${SOCI_BACKEND_POSTGRESQL_TARGET})
