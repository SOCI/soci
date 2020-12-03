cmake_minimum_required(VERSION 2.8.12)

project(conanSociTests CXX)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(sqlite3)

set(TEST_NAME main.tests)
add_executable(${TEST_NAME} main.test.cpp )

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)
conan_target_link_libraries(${TEST_NAME} ${CONAN_LIBS})

message("-- Conan libs: ${CONAN_LIBS}")

enable_testing()
add_test(NAME ${TEST_NAME}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMAND ${TEST_NAME})
