include(soci_utils)

# Defines a CMake target for a database backend.
#
# This function takes care of orchestrating the boilerplate that is needed in order to set up
# a library target as used for different DB backends. Accepted arguments are
#
# ALIAS_NAME   <name>                      Alias to use for the library. The alias name will be prefixed with "SOCI::"
# BACKEND_NAME <name>                      Name of the backend
# ENABLED_VARIABLE <variable>              CMake variable that indicates whether this backend is enabled. Will be set
#                                          to OFF if one of the dependencies are not satisfied.
# MISSING_DEPENDENCY_BEHAVIOR <behavior>   What to do if a dependency is not found. Valid values are "ERROR" and "DISABLE"
# TARGET_NAME <target>                     Name of the CMake target that shall be created for this backend
# DEPENDENCIES <spec1> [... <specN>]       List of dependency specifications. Each specification has to be a single
#                                          argument (single string) following the syntax
#                                          <find_spec> YIELDS <targets> [DEFINES <macros>]
#                                          where <find_spec> will be passed to find_package to find the dependency. Upon
#                                          success, all targets defined in <targets> are expected to exist. If provided,
#                                          all defines specified in <macros> will be added as public compile definitions
#                                          to the backend library if the dependency has been found.
#                                          For now, all dependencies are expected to be public and required.
# FIND_PACKAGE_FILES <file1> [... <fileN>] List of files used by find_package to locate one of the dependencies. Specified
#                                          files will be installed alongside SOCI in order to be usable from the install tree.
# HEADER_FILES <file1> [... <fileN>]       List of public header files associated with this backend target.
# REQUIRED_COMPONENTS <cmp1> [... <cmpN>]  List of SOCI components (full target names) to link this backend target to
# SOURCE_FILES <file1> [... <fileN>]       List of source files that shall be part of this backend component
function(soci_define_backend_target)
  set(FLAGS "")
  set(ONE_VAL_OPTIONS
    "ALIAS_NAME"
    "BACKEND_NAME"
    "ENABLED_VARIABLE"
    "MISSING_DEPENDENCY_BEHAVIOR"
    "TARGET_NAME"
  )
  set(MULTI_VAL_OPTIONS
    "DEPENDENCIES"
    "FIND_PACKAGE_FILES"
    "HEADER_FILES"
    "REQUIRED_COMPONENTS"
    "SOURCE_FILES"
  )
  cmake_parse_arguments(DEFINE_BACKEND "${FLAGS}" "${ONE_VAL_OPTIONS}" "${MULTI_VAL_OPTIONS}" ${ARGV})

  soci_verify_parsed_arguments(
    PREFIX "DEFINE_BACKEND"
    FUNCTION_NAME "soci_define_backend_target"
    REQUIRED "BACKEND_NAME" "SOURCE_FILES" "ENABLED_VARIABLE" "TARGET_NAME" "ALIAS_NAME"
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
  else()
    message(FATAL_ERROR "Invalid value '${DEFINE_BACKEND_MISSING_DEPENDENCY_BEHAVIOR}' for option 'MISSING_DEPENDENCY_BEHAVIOR'")
  endif()


  set(PUBLIC_DEP_CALL_ARGS "")

  foreach(CURRENT_DEP_SPEC IN LISTS DEFINE_BACKEND_DEPENDENCIES)
    if (NOT "${CURRENT_DEP_SPEC}" MATCHES "^([a-zA-Z0-9_:-;]+) YIELDS ([a-zA-Z0-9_:-;]+)( DEFINES [a-zA-Z0-9_;])?$")
      message(FATAL_ERROR "Invalid format for dependency specification in '${CURRENT_DEP_SPEC}'")
    endif()
    set(CURRENT_DEP_SEARCH ${CMAKE_MATCH_1})
    set(CURRENT_DEP_TARGETS ${CMAKE_MATCH_2})
    set(CURRENT_DEP_DEFINES ${CMAKE_MATCH_3})

    list(GET CURRENT_DEP_SEARCH 0 CURRENT_DEP)

    find_package(${CURRENT_DEP_SEARCH} ${REQUIRE_FLAG})

    if (NOT ${CURRENT_DEP}_FOUND)
      if (ERROR_ON_MISSING_DEPENDENCY)
        message(FATAL_ERROR "Expected find_package to error due to unmet dependency '${CURRENT_DEP}'")
      elseif(DISABLE_ON_MISSING_DEPENDENCY)
        message(STATUS "Disabling SOCI backend '${DEFINE_BACKEND_BACKEND_NAME}' due to unsatisfied dependency on '${CURRENT_DEP}'")

        # Set this backend to disabled by overwriting the corresponding cache variable
        # (without overwriting its description)
        get_property(DESCRIPTION CACHE "${DEFINE_BACKEND_ENABLED_VARIABLE}" PROPERTY HELPSTRING)
        set(${DEFINE_BACKEND_ENABLED_VARIABLE} OFF CACHE STRING "${DESCRIPTION}" FORCE)
        return()
      else()
        message(FATAL_ERROR "Unspecified handling of unmet dependency")
      endif()
    endif()

    foreach (CURRENT IN LISTS CURRENT_DEP_TARGETS)
      if (NOT TARGET "${CURRENT}")
        message(FATAL_ERROR "Expected successful find_package call with '${CURRENT_DEP_SEARCH}' to define target '${CURRENT}'")
      endif()
    endforeach()
    if (CURRENT_DEP_DEFINES)
      set(MACRO_NAMES_ARG "MACRO_NAMES ${CURRENT_DEP_DEFINES}")
    endif()
    list(APPEND PUBLIC_DEP_CALL_ARGS
      "NAME ${CURRENT_DEP} DEP_TARGETS ${CURRENT_DEP_TARGETS} TARGET SOCI::${DEFINE_BACKEND_ALIAS_NAME} ${MACRO_NAMES_ARG} REQUIRED"
    )
  endforeach()


  add_library(${DEFINE_BACKEND_TARGET_NAME} ${SOCI_LIB_TYPE} ${DEFINE_BACKEND_SOURCE_FILES})
  add_library(SOCI::${DEFINE_BACKEND_ALIAS_NAME} ALIAS ${DEFINE_BACKEND_TARGET_NAME})

  foreach(CURRENT_ARG_SET IN LISTS PUBLIC_DEP_CALL_ARGS)
    # Convert space-separated string to list
    string(REPLACE " " ";" CURRENT_ARG_SET "${CURRENT_ARG_SET}")
    soci_public_dependency(${CURRENT_ARG_SET})
  endforeach()
  foreach (CURRENT_DEP IN LISTS DEFINE_BACKEND_REQUIRED_COMPONENTS)
    target_link_libraries(${DEFINE_BACKEND_TARGET_NAME} PUBLIC "${CURRENT_DEP}")
  endforeach()

  target_include_directories(${DEFINE_BACKEND_TARGET_NAME} PRIVATE "${PROJECT_SOURCE_DIR}/include/private")

  set_target_properties(${DEFINE_BACKEND_TARGET_NAME}
    PROPERTIES
      SOVERSION ${PROJECT_VERSION_MAJOR}
      VERSION ${PROJECT_VERSION}
      EXPORT_NAME ${DEFINE_BACKEND_ALIAS_NAME}
  )

  if (DEFINE_BACKEND_HEADER_FILES)
    target_sources(${DEFINE_BACKEND_TARGET_NAME}
      PUBLIC
        FILE_SET headers TYPE HEADERS
        BASE_DIRS "${PROJECT_SOURCE_DIR}/include/"
        FILES ${DEFINE_BACKEND_HEADER_FILES}
    )
  endif()


  target_link_libraries(soci_interface INTERFACE SOCI::${DEFINE_BACKEND_ALIAS_NAME})


  # Setup installation rules for this backend
  install(
    TARGETS ${DEFINE_BACKEND_TARGET_NAME}
    EXPORT SOCI${DEFINE_BACKEND_BACKEND_NAME}Targets
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
    EXPORT SOCI${DEFINE_BACKEND_BACKEND_NAME}Targets
    DESTINATION "${SOCI_INSTALL_CMAKEDIR}"
    FILE SOCI${DEFINE_BACKEND_BACKEND_NAME}Targets.cmake
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
