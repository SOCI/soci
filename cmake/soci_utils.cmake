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

# This function declares a public dependency of a given target in such a way that
# the dependency is automatically populated to a generated SOCI config file.
#
# Use as
#   soci_public_dependency(
#      NAME <name>
#      DEP_TARGETS <dep target> ...
#      TARGET <target>
#   )
# where
# - <name> is the name of the dependency (used for lookup via find_package)
# - <dep target> is the name of the target that will be imported upon a
#                successful find_package call
# - <target> is the name of the ALIAS target to link the found dependency to
function(soci_public_dependency)
  soci_declare_dependency_impl(${ARGV} SCOPE PUBLIC)
endfunction()

# This function declares a private dependency of a given target in such a way that
# the dependency is automatically populated to a generated SOCI config file if needed.
#
# Use as
#   soci_private_dependency(
#      NAME <name>
#      DEP_TARGETS <dep target> ...
#      TARGET <target>
#   )
# where
# - <name> is the name of the dependency (used for lookup via find_package)
# - <dep target> is the name of the target that will be imported upon a
#                successful find_package call
# - <target> is the name of the ALIAS target to link the found dependency to
function(soci_private_dependency)
  soci_declare_dependency_impl(${ARGV} SCOPE PRIVATE)
endfunction()


# Initialize variables populated by soci_declare_dependency_impl
set(SOCI_DEPENDENCY_VARIABLES
  "SOCI_DEPENDENCY_SOCI_COMPONENTS"
  "SOCI_DEPENDENCY_NAMES"
  "SOCI_DEPENDENCY_TARGETS"
)
if (NOT DEFINED SOCI_UTILS_ALREADY_INCLUDED)
  foreach(VAR_NAME IN LISTS SOCI_DEPENDENCY_VARIABLES)
    set("${VAR_NAME}" "" CACHE INTERNAL "")
  endforeach()
endif()


# This function declares a dependency of a given target in such a way that
# the dependency is automatically populated to a generated SOCI config file if necessary.
# Normally, you shouldn't be calling this function directly and instead use one of
# soci_public_dependency() or soci_private_dependency().
#
# Use as
#   soci_declare_dependency_impl(
#      NAME <name>
#      DEP_TARGETS <dep target> ...
#      TARGET <target>
#      SCOPE [PUBLIC | PRIVATE]
#   )
# where
# - <name> is the name of the dependency (used for lookup via find_package)
# - <dep target> is the name of the target that will be imported upon a
#                successful find_package call
# - <target> is the name of the ALIAS target to link the found dependency to
# - <scope> is the scope (public or private) of the dependency
function(soci_declare_dependency_impl)
  set(ONE_VAL_OPTIONS "TARGET" "NAME" "SCOPE")
  set(MULTI_VAL_OPTIONS "DEP_TARGETS")

  cmake_parse_arguments(SOCI_DEP "${FLAGS}" "${ONE_VAL_OPTIONS}" "${MULTI_VAL_OPTIONS}" ${ARGV})
  soci_verify_parsed_arguments(
    PREFIX "SOCI_DEP"
    FUNCTION_NAME "soci_public_dependency"
    REQUIRED "TARGET" "NAME" "DEP_TARGETS" "SCOPE"
  )

  # Sanity checking
  if (NOT TARGET "${SOCI_DEP_TARGET}")
    message(FATAL_ERROR "Provided TARGET '${SOCI_DEP_TARGET}' does not exist")
  endif()

  get_target_property(UNDERLYING_TARGET "${SOCI_DEP_TARGET}" ALIASED_TARGET)
  if (NOT UNDERLYING_TARGET)
    message(FATAL_ERROR "Provided TARGET '${SOCI_DEP_TARGET}' is expected to be an ALIAS target")
  endif()
  get_target_property(UNDERLYING_TYPE "${UNDERLYING_TARGET}" TYPE)


  # Search for the package now
  set(SKIP_SEARCH ON)
  foreach (TGT IN LISTS SOCI_DEP_DEP_TARGETS)
    if (NOT TARGET "${TGT}")
      set(SKIP_SEARCH OFF)
      break()
    endif()
  endforeach()

  if (NOT SKIP_SEARCH)
    find_package("${SOCI_DEP_NAME}" REQUIRED)
  endif()

  set(INSTALLED_DEPENDENCIES "")
  set(FOUND_ONE OFF)
  foreach (TGT IN LISTS SOCI_DEP_DEP_TARGETS)
    if (NOT TARGET "${TGT}")
      if (FOUND_ONE)
        message(DEBUG "The following SOCI dependencies have been found only partially: ${SOCI_DEP_DEP_TARGETS}")
      endif()
      return()
    endif()

    set(FOUND_ONE ON)

    get_target_property(TGT_TYPE "${TGT}" TYPE)
    if ((NOT "${SOCI_DEP_SCOPE}" STREQUAL "PRIVATE") OR
      "${UNDERLYING_TYPE}" STREQUAL "STATIC_LIBRARY" OR NOT "${TGT_TYPE}" STREQUAL "STATIC_LIBRARY")
      # - Public (aka: non-private) dependencies are always required
      # - If SOCI is built as a static library, all dependencies (including static private) are required
      #   at link time and hence must be find_package'd by the installed config file
      # - Static libraries used only by SOCI internals are only needed when linking SOCI itself. Hence,
      #   if SOCI isn't built as a static library (which implies linking only happens at the downstream
      #   consumer) these dependencies don't have to be present when consuming the installed config file.
      list(APPEND INSTALLED_DEPENDENCIES "${TGT}")
    endif()

    target_link_libraries("${UNDERLYING_TARGET}" ${SOCI_DEP_SCOPE} "$<BUILD_INTERFACE:${TGT}>")
  endforeach()



  # Bookkeeping
  if (INSTALLED_DEPENDENCIES)
    list(APPEND SOCI_DEPENDENCY_SOCI_COMPONENTS "${SOCI_DEP_TARGET}")
    list(APPEND SOCI_DEPENDENCY_NAMES "${SOCI_DEP_NAME}")
    list(JOIN INSTALLED_DEPENDENCIES "|" STORED_TARGETS)
    list(APPEND SOCI_DEPENDENCY_TARGETS "${STORED_TARGETS}")

    foreach(VAR_NAME IN LISTS SOCI_DEPENDENCY_VARIABLES)
      set("${VAR_NAME}" "${${VAR_NAME}}" CACHE INTERNAL "")
    endforeach()
  endif()


endfunction()

set(SOCI_UTILS_ALREADY_INCLUDED TRUE)
