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
    message(FATAL_ERROR "${VERIFY_PARSED_ARGS_FUNCTION_NAME} - Received unrecognized options: ${${PREFIX}_UNPARSED_ARGUMENTS}")
  endif()
  if (DEFINED ${VERIFY_PARSED_ARGS_PREFIX}_KEYWORDS_MISSING_VALUES)
    message(FATAL_ERROR "${VERIFY_PARSED_ARGS_FUNCTION_NAME} - The following options are missing arguments: ${${PREFIX}_KEYWORDS_MISSING_VALUES}")
  endif()

  if (DEFINED VERIFY_PARSED_ARGS_REQUIRED)
    foreach(CURRENT_ARG IN LISTS VERIFY_PARSED_ARGS_REQUIRED)
      set(CURRENT_ARG "${VERIFY_PARSED_ARGS_PREFIX}_${CURRENT_ARG}")
      if (NOT DEFINED ${CURRENT_ARG})
        message(FATAL_ERROR "${VERIFY_PARSED_ARGS_FUNCTION_NAME - Missing required option '${CURRENT_ARG}'")
      endif()
    endforeach()
  endif()
endfunction()
