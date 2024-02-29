

add_library(soci_compiler_interface INTERFACE)

#
# Force compilation flags and set desired warnings level
#

option(SOCI_ENABLE_WERROR "Enables turning compiler warnings into errors" OFF)

if (MSVC)
  target_compile_definitions(soci_compiler_interface INTERFACE _CRT_SECURE_NO_DEPRECATE)
  target_compile_definitions(soci_compiler_interface INTERFACE _CRT_SECURE_NO_WARNINGS)
  target_compile_definitions(soci_compiler_interface INTERFACE _CRT_NONSTDC_NO_WARNING)
  target_compile_definitions(soci_compiler_interface INTERFACE _SCL_SECURE_NO_WARNINGS)

  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /we4266")
  endif()

  if (SOCI_ENABLE_WERROR)
    target_compile_options(soci_compiler_interface INTERFACE "/WX")
  endif()

  target_compile_options(soci_compiler_interface INTERFACE "/bigobj")
else()

  if (SOCI_ENABLE_WERROR)
    target_compile_options(soci_compiler_interface INTERFACE "-Werror")
  endif()


  if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_CXX_COMPILER}" MATCHES "clang")
    if(SOCI_ASAN AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 3.1)
      # This can also be used to set a linker flag
      target_link_libraries(soci_compiler_interface INTERFACE "-fsanitize=address")
      target_compile_options(soci_compiler_interface INTERFACE "-fsanitize=address")
    endif()

    set(SOCI_USE_STD_FLAGS ON)
  elseif (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
    if (SOCI_ASAN AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 4.8)
      # This can also be used to set a linker flag
      target_link_libraries(soci_compiler_interface INTERFACE "-fsanitize=address")
      target_compile_options(soci_compiler_interface INTERFACE "-fsanitize=address")
    endif()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SOCI_GCC_CLANG_COMMON_FLAGS} ")
    if (CMAKE_COMPILER_IS_GNUCXX)
      if (NOT (CMAKE_SYSTEM_NAME MATCHES "FreeBSD"))
        target_compile_options(soci_compiler_interface INTERFACE "-Wno-variadic-macros")
      endif()
    endif()

    set(SOCI_USE_STD_FLAGS ON)
  else()
	  message(WARNING "Unknown toolset - using default flags to build SOCI")
  endif()

  if (SOCI_USE_STD_FLAGS)
    target_compile_options(soci_compiler_interface
      INTERFACE
        "-pedantic"
        "-Wno-error=parentheses"
        "-Wall"
        "-Wextra"
        "-Wpointer-arith"
        "-Wcast-align"
        "-Wcast-qual"
        "-Wfloat-equal"
        "-Woverloaded-virtual"
        "-Wredundant-decls"
        "-Wno-long-long"
    )
  endif()
endif()
