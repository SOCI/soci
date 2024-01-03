include(soci_utils)

# This function extracts the current SOCI version from the version header file
#
# Use as
#    soci_parse_version(
#        ROOT_DIR <root_dir>
#        OUTPUT_VARIABLE <var_name>
#    )
# where
# - <root_dir> is the path to SOCI's source tree root directory
# - <var_name> is the name of the variable used to make the parsed version available. This variable
#   will contain the version in a cmake-compatible format
function(soci_parse_version)
  set(FLAGS "")
  set(ONE_VAL_OPTIONS "OUTPUT_VARIABLE" "ROOT_DIR")
  set(MULTI_VAL_OPTIONS "")
  cmake_parse_arguments(PARSE_VERSION "${FLAGS}" "${ONE_VAL_OPTIONS}" "${MULTI_VAL_OPTIONS}" ${ARGV})
  soci_verify_parsed_arguments(
    PREFIX "PARSE_VERSION"
    FUNCTION_NAME "soci_parse_version"
    REQUIRED "OUTPUT_VARIABLE" "ROOT_DIR"
  )

  set(VERSION_REGEX "^#define[ \\t]*SOCI_VERSION[ \\t]*([0-9]+)")

  file(STRINGS "${PARSE_VERSION_ROOT_DIR}/include/soci/version.h" VERSION_LINE
    REGEX ${VERSION_REGEX}
    LIMIT_COUNT 1
  )

  if (NOT VERSION_LINE MATCHES "${VERSION_REGEX}")
    message(STATUS "${PARSE_VERSION_ROOT_DIR}")
    message(FATAL_ERROR "Failed at parsing version from header file")
  endif()

  set(RAW_VERSION "${CMAKE_MATCH_1}")
  math(EXPR MAJOR_VERSION "${RAW_VERSION} / 100000")
  math(EXPR MINOR_VERSION "(${RAW_VERSION} / 100) % 1000")
  math(EXPR PATCH_VERSION "${RAW_VERSION} % 100")

  set("${PARSE_VERSION_OUTPUT_VARIABLE}" "${MAJOR_VERSION}.${MINOR_VERSION}.${PATCH_VERSION}" PARENT_SCOPE)
endfunction()
