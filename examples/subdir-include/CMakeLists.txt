# This is a very simple example of using SOCI in a CMake-based project
# when placing SOCI in a subdirectory lib/soci/ of your project.
# For this example, the SOCI backend called Empty is used.

cmake_minimum_required(VERSION 3.23 FATAL_ERROR)

project(SOCIExampleSubdirectoryInclude)

set(SOCI_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/soci)

# All compile options need to be set before the SOCI directory gets included.
# The backend you want to use needs to be enabled here.
set(SOCI_EMPTY ON CACHE INTERNAL "" FORCE)

# In order to not run into cross-platform issues, the **top-level** project
# should define these variables to ensure all build artefacts (of a given kind)
# end up in the same place.
if (PROJECT_IS_TOP_LEVEL)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
endif()

# This line needs to be changed to include the directory you placed SOCI in, e.g.
#  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/soci ${SOCI_BINARY_DIR})
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../.. ${SOCI_BINARY_DIR})

add_executable(subdir_include subdir-include.cpp)

# Link to SOCI (SOCI::soci will resolve all requested dependencies automatically)
target_link_libraries(subdir_include
    PRIVATE
      SOCI::soci
)
