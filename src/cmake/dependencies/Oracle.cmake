set(ORACLE_FIND_QUIETLY TRUE)

find_package(Oracle)

boost_external_report(Oracle INCLUDE_DIR LIBRARIES)

if(ORACLE_FOUND)
  include_directories(${ORACLE_INCLUDE_DIR})
  add_definitions(-DHAVE_ORACLE)
endif()
