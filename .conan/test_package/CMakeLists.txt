cmake_minimum_required(VERSION 2.8.12)
project(example CXX)

set(CMAKE_CXX_STANDARD 17)

add_executable(${PROJECT_NAME} example.cpp)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)
conan_target_link_libraries(${PROJECT_NAME} ${CONAN_LIBS})

message("-- Conan libs: ${CONAN_LIBS}")
