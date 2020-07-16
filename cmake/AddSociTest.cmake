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
  target_include_directories(soci_${ARG_NAME}_test PRIVATE ${PROJECT_SOURCE_DIR}/include/private)

  if(SOCI_WITH_BOOST)
    target_link_libraries(soci_${ARG_NAME}_test PRIVATE Boost::boost Boost::date_time)
    target_compile_definitions(soci_${ARG_NAME}_test PRIVATE SOCI_HAVE_BOOST)
  endif()


  add_test(NAME soci_${ARG_NAME} COMMAND soci_${ARG_NAME}_test ${ARG_ARGUMENTS})
endfunction()