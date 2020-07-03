function(add_soci_test)
  cmake_parse_arguments(
    ARG
    ""
    "NAME"
    ""
    ${ARGN}
  )

  add_executable(soci_${ARG_NAME}_test)
  target_link_libraries(soci_${ARG_NAME}_test PRIVATE soci_core soci_${ARG_NAME})
  target_include_directories(soci_${ARG_NAME}_test 
    PRIVATE 
      ${SOCI_SOURCE_DIR}/include/private 
      ${CMAKE_CURRENT_LIST_DIR}
  )

  add_test(NAME soci_${ARG_NAME} COMMAND soci_${ARG_NAME}_test)
endfunction()