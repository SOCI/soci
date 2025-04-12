# This function is meant to be called by other cmake functions that use the cmake_parse_arguments function.
# It implements the boilerplate that is associated with validating the parsed arguments (e.g. no unknown arguments used;
# ensuring all required options have been set).
#
# Use as
#     soci_verify_parsed_arguments(
#         PREFIX <prefix>
#         FUNCTION_NAME <name>
#         [REQUIRED <option1> <option2> …]
#     )
# where
# - <prefix> is the prefix used in the cmake_parse_arguments call that shall be validated
# - <name> is the name of the calling function or macro - this will be part of error mesages
# - <optionN> are names of options **without** the prefix
function(soci_verify_parsed_arguments)
  set(FLAGS "")
  set(ONE_VAL_OPTIONS "PREFIX" "FUNCTION_NAME")
  set(MULTI_VAL_OPTIONS "REQUIRED")
  cmake_parse_arguments(VERIFY_PARSED_ARGS "${FLAGS}" "${ONE_VAL_OPTIONS}" "${MULTI_VAL_OPTIONS}" ${ARGV})

  # First, verify own arguments
  if (DEFINED VERIFY_PARSED_ARGS_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "verify_parsed_arguments - Received unrecognized options: ${VERIFY_PARSED_ARGS_UNPARSED_ARGUMENTS}")
  endif()
  if (DEFINED VERIFY_PARSED_ARGS_KEYWORDS_MISSING_VALUES)
    message(FATAL_ERROR "verify_parsed_arguments - The following options are missing argumens: ${VERIFY_PARSED_ARGS_KEYWORDS_MISSING_VALUES}")
  endif()
  if (NOT DEFINED VERIFY_PARSED_ARGS_PREFIX)
    message(FATAL_ERROR "verify_parsed_arguments - Missing required option 'PREFIX'")
  endif()
  if (NOT DEFINED VERIFY_PARSED_ARGS_FUNCTION_NAME)
    message(FATAL_ERROR "verify_parsed_arguments - Missing required option 'FUNCTION_NAME'")
  endif()

  # Now start the function's actual job: Verifying a top-level function's call to cmake_parse_arguments
  if (DEFINED ${VERIFY_PARSED_ARGS_PREFIX}_UNPARSED_ARGUMENTS)
    message(FATAL_ERROR "${VERIFY_PARSED_ARGS_FUNCTION_NAME} - Received unrecognized options: ${${VERIFY_PARSED_ARGS_PREFIX}_UNPARSED_ARGUMENTS}")
  endif()
  if (DEFINED ${VERIFY_PARSED_ARGS_PREFIX}_KEYWORDS_MISSING_VALUES)
    message(FATAL_ERROR "${VERIFY_PARSED_ARGS_FUNCTION_NAME} - The following options are missing arguments: ${${VERIFY_PARSED_ARGS_PREFIX}_KEYWORDS_MISSING_VALUES}")
  endif()

  if (DEFINED VERIFY_PARSED_ARGS_REQUIRED)
    foreach(CURRENT_ARG IN LISTS VERIFY_PARSED_ARGS_REQUIRED)
      set(CURRENT_ARG "${VERIFY_PARSED_ARGS_PREFIX}_${CURRENT_ARG}")
      if (NOT DEFINED ${CURRENT_ARG})
        message(FATAL_ERROR "${VERIFY_PARSED_ARGS_FUNCTION_NAME} - Missing required option '${CURRENT_ARG}'")
      endif()
    endforeach()
  endif()
endfunction()


# Initialize variables populated by soci_public_dependency
set(SOCI_DEPENDENCY_VARIABLES
  "SOCI_DEPENDENCY_SOCI_COMPONENTS"
  "SOCI_DEPENDENCY_NAMES"
  "SOCI_DEPENDENCY_TARGETS"
  "SOCI_DEPENDENCY_REQUIRED"
  "SOCI_DEPENDENCY_MACRO_NAMES"
  "SOCI_DEPENDENCY_COMPONENTS"
)
if (NOT DEFINED SOCI_UTILS_ALREADY_INCLUDED)
  foreach(VAR_NAME IN LISTS SOCI_DEPENDENCY_VARIABLES)
    set("${VAR_NAME}" "" CACHE INTERNAL "")
  endforeach()
endif()

# This function declares a public dependency of a given target in such a way that
# the dependency is automatically populated to a generated SOCI config file.
#
# Use as
#   soci_public_dependency(
#      NAME <name>
#      DEP_TARGET <dep target>
#      TARGET <target>
#      [MACRO_NAMES <macro name> ...]
#      [REQUIRED]
#      [COMPONENTS <component> ...]
#   )
# where
# - <name> is the name of the dependency (used for lookup via find_package)
# - <dep target> is the name of the target that will be imported upon a
#                successful find_package call
# - <target> is the name of the ALIAS target to link the found dependency to
# - <component> are the names of specific components of the dependency
function(soci_public_dependency)
  set(FLAGS "REQUIRED")
  set(ONE_VAL_OPTIONS "TARGET" "NAME")
  set(MULTI_VAL_OPTIONS "COMPONENTS" "MACRO_NAMES" "DEP_TARGETS")

  cmake_parse_arguments(PUBLIC_DEP "${FLAGS}" "${ONE_VAL_OPTIONS}" "${MULTI_VAL_OPTIONS}" ${ARGV})
  soci_verify_parsed_arguments(
    PREFIX "PUBLIC_DEP"
    FUNCTION_NAME "soci_public_dependency"
    REQUIRED "TARGET" "NAME" "DEP_TARGETS"
  )

  # Sanity checking
  if (NOT TARGET "${PUBLIC_DEP_TARGET}")
    message(FATAL_ERROR "Provided TARGET '${PUBLIC_DEP_TARGET}' does not exist")
  endif()

  get_target_property(UNDERLYING_TARGET "${PUBLIC_DEP_TARGET}" ALIASED_TARGET)
  if (NOT UNDERLYING_TARGET)
    message(FATAL_ERROR "Provided TARGET '${PUBLIC_DEP_TARGET}' is expected to be an ALIAS target")
  endif()


  # Bookkeeping
  list(APPEND SOCI_DEPENDENCY_SOCI_COMPONENTS "${PUBLIC_DEP_TARGET}")
  list(APPEND SOCI_DEPENDENCY_NAMES "${PUBLIC_DEP_NAME}")
  list(JOIN PUBLIC_DEP_DEP_TARGETS "|" STORED_TARGETS)
  list(APPEND SOCI_DEPENDENCY_TARGETS "${STORED_TARGETS}")
  list(APPEND SOCI_DEPENDENCY_REQUIRED "${PUBLIC_DEP_REQUIRED}")
  if (PUBLIC_DEP_MACRO_NAMES)
    list(JOIN PUBLIC_DEP_MACRO_NAMES "|" STORED_MACROS)
  else()
    set(STORED_MACROS "")
  endif()
  list(APPEND SOCI_DEPENDENCY_MACRO_NAMES "${STORED_MACROS}")
  if (PUBLIC_DEP_COMPONENTS)
    list(JOIN PUBLIC_DEP_COMPONENTS "|" STORED_COMPONENTS)
  else()
    set(STORED_COMPONENTS "")
  endif()
  list(APPEND SOCI_DEPENDENCY_COMPONENTS "${STORED_COMPONENTS}")


  foreach(VAR_NAME IN LISTS SOCI_DEPENDENCY_VARIABLES)
    set("${VAR_NAME}" "${${VAR_NAME}}" CACHE INTERNAL "")
  endforeach()


  # Search for the package now
  if (PUBLIC_DEP_REQUIRED)
    set(REQUIRED "REQUIRED")
  else()
    set(REQUIRED "")
  endif()

  if (PUBLIC_DEP_COMPONENTS)
    set(COMPONENTS COMPONENTS ${PUBLIC_DEP_COMPONENTS})
  else()
    set(COMPONENTS "")
  endif()

  set(SKIP_SEARCH ON)
  foreach (TGT IN LISTS PUBLIC_DEP_DEP_TARGETS)
    if (NOT TARGET "${TGT}")
      set(SKIP_SEARCH OFF)
    endif()
  endforeach()

  if (NOT SKIP_SEARCH)
    find_package("${PUBLIC_DEP_NAME}" ${COMPONENTS} ${REQUIRED})
  endif()

  set(FOUND_ONE OFF)
  foreach (TGT IN LISTS PUBLIC_DEP_DEP_TARGETS)
    if (NOT TARGET "${TGT}")
      if (FOUND_ONE)
        message(DEBUG "The following SOCI dependencies have been found only partially: ${PUBLIC_DEP_DEP_TARGETS}")
      endif()
      return()
    endif()

    set(FOUND_ONE ON)

    target_link_libraries("${UNDERLYING_TARGET}" PUBLIC "$<BUILD_INTERFACE:${TGT}>")
  endforeach()

  foreach (MACRO_NAME IN LISTS PUBLIC_DEP_MACRO_NAMES)
    # Note: We don't want these compile definitions to be exported to the install tree
    # -> We put the logic of when they should be defined into the cmake config files
    target_compile_definitions("${UNDERLYING_TARGET}" PUBLIC "$<BUILD_INTERFACE:${MACRO_NAME}>")
  endforeach()
endfunction()

# This function can be used to check whether two C++ types actually refer to the same
# type (e.g. if one is aliased to the other).
#
# Use as
#     soci_are_types_same(
#         TYPES <type1> <type2> [... <typeN>]
#         OUTPUT_VARIABLE <name>
#     )
# where
# - <type1>, <type2>, <typeN> are the types to test
# - <name> is the name of the variable that will hold the result of the check
function(soci_are_types_same)
  set(FLAGS "")
  set(ONE_VAL_OPTIONS "OUTPUT_VARIABLE")
  set(MULTI_VAL_OPTIONS "TYPES")

  cmake_parse_arguments(TYPES_SAME "${FLAGS}" "${ONE_VAL_OPTIONS}" "${MULTI_VAL_OPTIONS}" ${ARGV})
  soci_verify_parsed_arguments(
    PREFIX "TYPES_SAME"
    FUNCTION_NAME "soci_are_types_same"
    REQUIRED "TYPES" "OUTPUT_VARIABLE"
  )

  set(TEST_CODE "#include <cstdint>\nstruct Foo { ")
  set(TEST_NAME "")
  foreach(CURRENT_TYPE IN LISTS TYPES_SAME_TYPES)
    string(APPEND TEST_CODE "void foo(${CURRENT_TYPE} x); ")
    string(TOUPPER "${CURRENT_TYPE}" UPPER_TYPE)
    string(APPEND TEST_NAME "${UPPER_TYPE}_")
  endforeach()
  string(APPEND TEST_CODE "};\nint main() {}")
  string(APPEND TEST_NAME "ARE_DISTINGUISHABLE")

  include(CheckCXXSourceCompiles)

  # If some of the provided types are actually the same, compilation
  # will fail due to duplication of function declarations.
  check_cxx_source_compiles("${TEST_CODE}" ${TEST_NAME})

  if (${TEST_NAME})
    set("${TYPES_SAME_OUTPUT_VARIABLE}" FALSE PARENT_SCOPE)
  else()
    set("${TYPES_SAME_OUTPUT_VARIABLE}" TRUE PARENT_SCOPE)
  endif()
endfunction()

set(SOCI_UTILS_ALREADY_INCLUDED TRUE)
