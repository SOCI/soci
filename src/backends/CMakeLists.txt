colormsg(_HIBLUE_ "Configuring SOCI database backends:")

# First, we'll investigate what can be found from database engines
foreach(dep ${SOCI_BACKENDS_DB_DEPENDENCIES})
	string(TOUPPER ${dep} depUP)
	find_package(${dep})
	if(${dep}_FOUND OR ${depUP}_FOUND)
		set(${depUP}_FOUND ON)
	else()
		set(${depUP}_FOUND OFF)
	endif()
endforeach()

# get all files in backends
file(GLOB backend_dirs RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *)

# empty backend always on by default
option(EMPTY_FOUND "Build empty backend" ON)

# enable only found backends
foreach(dir ${backend_dirs})
	if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${dir})
		if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${dir}/CMakeLists.txt)
			string(TOUPPER ${dir} dirUP)
			if(${dirUP}_FOUND)
				add_subdirectory(${dir})
				message("added backend: ${dir}")
			endif()
		endif()
	endif()
endforeach()

message(STATUS "")
