option(SOCI_ASAN "Enable address sanitizer on GCC v4.8+/Clang v 3.1+" OFF)

if (MSVC)
  add_definitions(-D_CRT_SECURE_NO_DEPRECATE)
  add_definitions(-D_CRT_SECURE_NO_WARNINGS)
  add_definitions(-D_CRT_NONSTDC_NO_WARNING)
  add_definitions(-D_SCL_SECURE_NO_WARNINGS)

  if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4 /we4266")
  endif()
else()
  set(SOCI_GCC_CLANG_COMMON_FLAGS
    "-pedantic -Werror -Wno-error=parentheses -Wall -Wextra -Wpointer-arith -Wcast-align -Wcast-qual -Wfloat-equal -Woverloaded-virtual -Wredundant-decls -Wno-long-long")

  if (SOCI_CXX11)
    set(SOCI_CXX_VERSION_FLAGS "-std=c++11")
  else()
    set(SOCI_CXX_VERSION_FLAGS "-std=gnu++98")
  endif()

  if("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang" OR "${CMAKE_CXX_COMPILER}" MATCHES "clang")

    if(NOT CMAKE_CXX_COMPILER_VERSION LESS 3.1 AND SOCI_ASAN)
      set(SOCI_GCC_CLANG_COMMON_FLAGS "${SOCI_GCC_CLANG_COMMON_FLAGS} -fsanitize=address")
    endif()

    # enforce C++11 for Clang
    set(SOCI_WITH_CXX11 ON)
    set(SOCI_CXX_VERSION_FLAGS "-std=c++11")
    add_definitions(-DCATCH_CONFIG_CPP11_NO_IS_ENUM)

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SOCI_GCC_CLANG_COMMON_FLAGS} ${SOCI_CXX_VERSION_FLAGS}")

  elseif(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)

    if(NOT CMAKE_CXX_COMPILER_VERSION LESS 4.8 AND SOCI_ASAN)
      set(SOCI_GCC_CLANG_COMMON_FLAGS "${SOCI_GCC_CLANG_COMMON_FLAGS} -fsanitize=address")
    endif()

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SOCI_GCC_CLANG_COMMON_FLAGS} ${SOCI_CXX_VERSION_FLAGS} ")
    if (CMAKE_COMPILER_IS_GNUCXX)
        if (CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        else()
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-variadic-macros")
        endif()
    endif()

  else()
	message(WARNING "Unknown toolset - using default flags to build SOCI")
  endif()

endif()