# SQLite3 tests target
set(BACKEND_TEST_NAME conan.soci.sqlite3.tests)
add_executable(${BACKEND_TEST_NAME} ../main.test.cpp sqlite3.test.cpp)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)
conan_target_link_libraries(${BACKEND_TEST_NAME} ${CONAN_LIBS})

message("-- Conan libs: ${CONAN_LIBS}")

enable_testing()
add_test(NAME ${BACKEND_TEST_NAME}
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMAND ${BACKEND_TEST_NAME})
