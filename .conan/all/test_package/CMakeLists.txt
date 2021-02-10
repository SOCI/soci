cmake_minimum_required(VERSION 3.1)

project(conanSociTests CXX)

set(TEST_NAME main.tests)
add_executable(${TEST_NAME} main.test.cpp )
target_compile_features(${TEST_NAME} PUBLIC cxx_std_17)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)
conan_target_link_libraries(${TEST_NAME} ${CONAN_LIBS})

#message("-- Conan libs: ${CONAN_LIBS}")

add_subdirectory(none)
add_subdirectory(empty)
add_subdirectory(sqlite3)
#add_subdirectory(odbc)
#add_subdirectory(mysql)

enable_testing()
add_test(NAME ${TEST_NAME}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMAND ${TEST_NAME})
