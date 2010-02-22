set(SOCI_TEST_EMPTY_TARGET test_empty)
set(SOCI_TEST_EMPTY_SOURCES test-empty.cpp)

include_directories(${SOCI_SOURCE_DIR}/core/test)
include_directories(${SOCI_SOURCE_DIR}/backends/empty)

add_executable(${SOCI_TEST_EMPTY_TARGET} ${SOCI_TEST_EMPTY_SOURCES})

target_link_libraries(${SOCI_TEST_EMPTY_TARGET}
  ${SOCI_CORE_TARGET}
  ${SOCI_BACKEND_EMPTY_TARGET})
