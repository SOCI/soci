function(add_soci_test)
  cmake_parse_arguments(
    ARG
    ""
    "NAME"
    "ARGUMENTS"
    ${ARGN}
  )

  add_executable(soci_${ARG_NAME}_test)
  target_link_libraries(soci_${ARG_NAME}_test PRIVATE SOCI::soci_core SOCI::soci_${ARG_NAME} Catch::Catch)
  target_include_directories(soci_${ARG_NAME}_test PRIVATE ${SOCI_SOURCE_DIR}/include/private ${CMAKE_CURRENT_LIST_DIR})

  add_test(NAME soci_${ARG_NAME} COMMAND soci_${ARG_NAME}_test ${ARG_ARGUMENTS})
endfunction()