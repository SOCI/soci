include(soci_utils)

# Defines a CMake target for a database backend.
#
# This function takes care of orchestrating the boilerplate that is needed in order to set up
# a library target as used for different DB backends. The corresponding
# SOCI_<uppercase-name>_SOURCE macro is defined during compilation to control
# symbol visibility and for build-time use for conditional compilation.
#
# Accepted arguments are the following:
#
# NAME <name>                              Name of the backend. This function will create an alias target using this name with "SOCI::" prefix.
# MISSING_DEPENDENCY_BEHAVIOR <behavior>   What to do if a dependency is not found. Valid values are "ERROR", "DISABLE" and "BUILTIN".
# TARGET_NAME <target>                     Name of the CMake target that shall be created for this backend
# DEPENDENCIES <spec1> [... <specN>]       List of dependency specifications. Each specification has to be a single
#                                          argument (single string) following the syntax
#                                          <find_spec> YIELDS <targets>
#                                          where <find_spec> will be passed to find_package to find the dependency. Upon
#                                          success, all targets defined in <targets> are expected to exist.
#                                          For now, all dependencies are expected to be public and required.
# FIND_PACKAGE_FILES <file1> [... <fileN>] List of files used by find_package to locate one of the dependencies. Specified
#                                          files will be installed alongside SOCI in order to be usable from the install tree.
# HEADER_FILES <file1> [... <fileN>]       List of public header files associated with this backend target.
# SOURCE_FILES <file1> [... <fileN>]       List of source files that shall be part of this backend component
#
# It sets SOCI_<NAME> cache variable to ON if the backend dependencies are available and OFF otherwise.
function(soci_define_backend_target)
  set(FLAGS "")
  set(ONE_VAL_OPTIONS
    "NAME"
    "MISSING_DEPENDENCY_BEHAVIOR"
    "TARGET_NAME"
  )
  set(MULTI_VAL_OPTIONS
    "DEPENDENCIES"
    "FIND_PACKAGE_FILES"
    "HEADER_FILES"
    "SOURCE_FILES"
  )
  cmake_parse_arguments(DEFINE_BACKEND "${FLAGS}" "${ONE_VAL_OPTIONS}" "${MULTI_VAL_OPTIONS}" ${ARGV})

  soci_verify_parsed_arguments(
    PREFIX "DEFINE_BACKEND"
    FUNCTION_NAME "soci_define_backend_target"
    REQUIRED "NAME" "SOURCE_FILES" "TARGET_NAME"
  )

  if (NOT DEFINE_BACKEND_MISSING_DEPENDENCY_BEHAVIOR)
    set(DEFINE_BACKEND_MISSING_DEPENDENCY_BEHAVIOR "ERROR")
  else()
    string(TOUPPER "${DEFINE_BACKEND_MISSING_DEPENDENCY_BEHAVIOR}" DEFINE_BACKEND_MISSING_DEPENDENCY_BEHAVIOR)
  endif()


  if (DEFINE_BACKEND_MISSING_DEPENDENCY_BEHAVIOR STREQUAL "ERROR")
    set(REQUIRE_FLAG "REQUIRED")
    set(ERROR_ON_MISSING_DEPENDENCY ON)
  elseif(DEFINE_BACKEND_MISSING_DEPENDENCY_BEHAVIOR STREQUAL "DISABLE")
    set(DISABLE_ON_MISSING_DEPENDENCY ON)
  elseif(DEFINE_BACKEND_MISSING_DEPENDENCY_BEHAVIOR STREQUAL "BUILTIN")
    set(REQUIRE_FLAG "QUIET")
    set(BUILTIN_ON_MISSING_DEPENDENCY ON)
  else()
    message(FATAL_ERROR "Invalid value '${DEFINE_BACKEND_MISSING_DEPENDENCY_BEHAVIOR}' for option 'MISSING_DEPENDENCY_BEHAVIOR'")
  endif()


  set(PUBLIC_DEP_CALL_ARGS "")

  # This variable indicates whether the backend is enabled or not.
  string(TOUPPER "${DEFINE_BACKEND_NAME}" BACKEND_UPPER)
  set(DEFINE_BACKEND_ENABLED_VARIABLE "SOCI_${BACKEND_UPPER}")

  # Skip looking for dependencies if built-in version is preferred.
  if (${SOCI_${BACKEND_UPPER}_BUILTIN})
    unset(DEFINE_BACKEND_DEPENDENCIES)
  endif()

  foreach(CURRENT_DEP_SPEC IN LISTS DEFINE_BACKEND_DEPENDENCIES)
    if (NOT "${CURRENT_DEP_SPEC}" MATCHES "^([a-zA-Z0-9_:-;]+) YIELDS ([a-zA-Z0-9_:-;]+)$")
      message(FATAL_ERROR "Invalid format for dependency specification in '${CURRENT_DEP_SPEC}'")
    endif()
    set(CURRENT_DEP_SEARCH ${CMAKE_MATCH_1})
    set(CURRENT_DEP_TARGETS ${CMAKE_MATCH_2})

    list(GET CURRENT_DEP_SEARCH 0 CURRENT_DEP)

    find_package(${CURRENT_DEP_SEARCH} ${REQUIRE_FLAG})

    if (NOT ${CURRENT_DEP}_FOUND)
      if (ERROR_ON_MISSING_DEPENDENCY)
        message(FATAL_ERROR "Expected find_package to error due to unmet dependency '${CURRENT_DEP}'")
      elseif(DISABLE_ON_MISSING_DEPENDENCY)
        message(STATUS "Disabling SOCI backend '${DEFINE_BACKEND_NAME}' due to unsatisfied dependency on '${CURRENT_DEP}'")

        # Set this backend to disabled by overwriting the corresponding cache variable
        # (without overwriting its description)
        get_property(DESCRIPTION CACHE "${DEFINE_BACKEND_ENABLED_VARIABLE}" PROPERTY HELPSTRING)
        set(${DEFINE_BACKEND_ENABLED_VARIABLE} OFF CACHE STRING "${DESCRIPTION}" FORCE)
        return()
      elseif(BUILTIN_ON_MISSING_DEPENDENCY)
        break()
      else()
        message(FATAL_ERROR "Unspecified handling of unmet dependency")
      endif()
    else()
      # This is wasteful, but do it again without "QUIET" flag to show the result if it succeeded.
      if (REQUIRE_FLAG STREQUAL "QUIET")
        find_package(${CURRENT_DEP_SEARCH})
      endif()

      foreach (CURRENT IN LISTS CURRENT_DEP_TARGETS)
        if (NOT TARGET "${CURRENT}")
          message(FATAL_ERROR "Expected successful find_package call with '${CURRENT_DEP_SEARCH}' to define target '${CURRENT}'")
        endif()
      endforeach()
      list(APPEND PUBLIC_DEP_CALL_ARGS
        "NAME ${CURRENT_DEP} DEP_TARGETS ${CURRENT_DEP_TARGETS} TARGET SOCI::${DEFINE_BACKEND_NAME}"
      )
    endif()
  endforeach()


  add_library(${DEFINE_BACKEND_TARGET_NAME} ${SOCI_LIB_TYPE} ${DEFINE_BACKEND_SOURCE_FILES})
  add_library(SOCI::${DEFINE_BACKEND_NAME} ALIAS ${DEFINE_BACKEND_TARGET_NAME})
  # defined during compilation to control symbol visibility/export
  target_compile_definitions(${DEFINE_BACKEND_TARGET_NAME} PRIVATE SOCI_${BACKEND_UPPER}_SOURCE)

  foreach(CURRENT_ARG_SET IN LISTS PUBLIC_DEP_CALL_ARGS)
    # Convert space-separated string to list
    string(REPLACE " " ";" CURRENT_ARG_SET "${CURRENT_ARG_SET}")
    soci_public_dependency(${CURRENT_ARG_SET})
  endforeach()

  # Set settings common to all backends.
  target_link_libraries(${DEFINE_BACKEND_TARGET_NAME} PUBLIC SOCI::Core PRIVATE ${soci_fmt})
  target_include_directories(${DEFINE_BACKEND_TARGET_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/include/private")

  soci_build_library_name(full_backend_target_name "${DEFINE_BACKEND_TARGET_NAME}")

  set_target_properties(${DEFINE_BACKEND_TARGET_NAME}
    PROPERTIES
      OUTPUT_NAME ${full_backend_target_name}
      SOVERSION ${PROJECT_VERSION_MAJOR}
      VERSION ${PROJECT_VERSION}
      EXPORT_NAME ${DEFINE_BACKEND_NAME}
  )

  if (DEFINE_BACKEND_HEADER_FILES)
    target_sources(${DEFINE_BACKEND_TARGET_NAME}
      PUBLIC
        FILE_SET headers TYPE HEADERS
        BASE_DIRS "${PROJECT_SOURCE_DIR}/include/"
        FILES ${DEFINE_BACKEND_HEADER_FILES}
    )
  endif()


  target_link_libraries(soci_interface INTERFACE SOCI::${DEFINE_BACKEND_NAME})


  # Setup installation rules for this backend
  install(
    TARGETS ${DEFINE_BACKEND_TARGET_NAME}
    EXPORT SOCI${DEFINE_BACKEND_NAME}Targets
    RUNTIME DESTINATION "${SOCI_INSTALL_BINDIR}"
      COMPONENT soci_runtime
    LIBRARY DESTINATION "${SOCI_INSTALL_LIBDIR}"
      COMPONENT          soci_runtime
      NAMELINK_COMPONENT soci_development
    ARCHIVE DESTINATION "${SOCI_INSTALL_LIBDIR}"
      COMPONENT soci_development
    FILE_SET headers DESTINATION "${SOCI_INSTALL_INCLUDEDIR}"
      COMPONENT soci_development
  )
  # Generate and install a targets file
  install(
    EXPORT SOCI${DEFINE_BACKEND_NAME}Targets
    DESTINATION "${SOCI_INSTALL_CMAKEDIR}"
    FILE SOCI${DEFINE_BACKEND_NAME}Targets.cmake
    NAMESPACE SOCI::
    COMPONENT soci_development
  )

  foreach(FIND_FILE IN LISTS DEFINE_BACKEND_FIND_PACKAGE_FILES)
    install(
      FILES "${FIND_FILE}"
      DESTINATION "${SOCI_INSTALL_CMAKEDIR}/find_package_files/"
      COMPONENT soci_development
    )
  endforeach()
endfunction()
