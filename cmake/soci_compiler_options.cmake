include(CheckCXXCompilerFlag)


add_library(soci_compiler_interface INTERFACE)

#
# Force compilation flags and set desired warnings level
#

option(SOCI_ENABLE_WERROR "Enables turning compiler warnings into errors" OFF)
option(SOCI_ASAN "Enable building SOCI with enabled address sanitizers" OFF)
option(SOCI_UBSAN "Enable undefined behaviour sanitizer" OFF)
set(SOCI_LD "" CACHE STRING "Specify a non-default linker")


if (WIN32)
  target_compile_definitions(soci_compiler_interface
    INTERFACE
      _CRT_SECURE_NO_DEPRECATE
      _CRT_SECURE_NO_WARNINGS
      _CRT_NONSTDC_NO_WARNING
      _SCL_SECURE_NO_WARNINGS
  )

  # Prevent the Windows header files from defining annoying macros
  # and also cut down on the definitions in general
  target_compile_definitions(soci_compiler_interface
    INTERFACE
      NOMINMAX
      WIN32_LEAN_AND_MEAN
  )
endif()

if (MSVC)
  # Configure warnings
  target_compile_options(soci_compiler_interface
    INTERFACE
      "$<$<BOOL:${PROJECT_IS_TOP_LEVEL}>:/W4>"
      "$<$<BOOL:${PROJECT_IS_TOP_LEVEL}>:/we4266>"
  )

  if (SOCI_ENABLE_WERROR)
    target_compile_options(soci_compiler_interface INTERFACE "/WX")
  endif()

  if (SOCI_LD)
    message(FATAL_ERROR "Using a non-default linker is not supported when using MSVC")
  endif()

  target_compile_options(soci_compiler_interface INTERFACE "/bigobj" "/utf-8")
else()

  if (SOCI_ENABLE_WERROR)
    target_compile_options(soci_compiler_interface INTERFACE "-Werror")
  endif()

  if (SOCI_UBSAN)
    target_compile_options(soci_compiler_interface INTERFACE "-fsanitize=undefined")
    target_link_options(soci_compiler_interface INTERFACE "-fsanitize=undefined")
  endif()

  if (SOCI_LD)
    # CMake asks the compiler to do the linking so we have to pass the desired linker to the compiler
    set(USE_LD_FLAG "-fuse-ld=${SOCI_LD}")
    check_cxx_compiler_flag("${USE_LD_FLAG}" CAN_USE_CUSTOM_LD)
    if (NOT CAN_USE_CUSTOM_LD)
      message(FATAL_ERROR "Can't use custom linker '${SOCI_LD}' - compiler doesn't accept flag '${USE_LD_FLAG}'")
    endif()
    target_link_options(soci_compiler_interface INTERFACE "${USE_LD_FLAG}")
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

    if (CMAKE_COMPILER_IS_GNUCXX)
      if (NOT (CMAKE_SYSTEM_NAME MATCHES "FreeBSD"))
        target_compile_options(soci_compiler_interface INTERFACE "$<$<BOOL:${PROJECT_IS_TOP_LEVEL}>:-Wno-variadic-macros>")
      endif()
    endif()

    set(SOCI_USE_STD_FLAGS ON)
  else()
	  message(WARNING "Unknown toolset - using default flags to build SOCI")
  endif()

  if (SOCI_USE_STD_FLAGS)
    target_compile_options(soci_compiler_interface
      INTERFACE
        "$<$<BOOL:${PROJECT_IS_TOP_LEVEL}>:-pedantic>"
        "$<$<BOOL:${PROJECT_IS_TOP_LEVEL}>:-Wno-error=parentheses>"
        "$<$<BOOL:${PROJECT_IS_TOP_LEVEL}>:-Wall>"
        "$<$<BOOL:${PROJECT_IS_TOP_LEVEL}>:-Wextra>"
        "$<$<BOOL:${PROJECT_IS_TOP_LEVEL}>:-Wpointer-arith>"
        "$<$<BOOL:${PROJECT_IS_TOP_LEVEL}>:-Wcast-align>"
        "$<$<BOOL:${PROJECT_IS_TOP_LEVEL}>:-Wcast-qual>"
        "$<$<BOOL:${PROJECT_IS_TOP_LEVEL}>:-Wfloat-equal>"
        "$<$<BOOL:${PROJECT_IS_TOP_LEVEL}>:-Woverloaded-virtual>"
        "$<$<BOOL:${PROJECT_IS_TOP_LEVEL}>:-Wredundant-decls>"
        "$<$<BOOL:${PROJECT_IS_TOP_LEVEL}>:-Wno-long-long>"
    )
  endif()
endif()
