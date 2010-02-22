set(Boost_FIND_QUIETLY TRUE)

find_package(Boost 1.33.1 REQUIRED date_time)

set(Boost_RELEASE_VERSION
  "${Boost_MAJOR_VERSION}.${Boost_MINOR_VERSION}.${Boost_SUBMINOR_VERSION}")

boost_external_report(Boost RELEASE_VERSION INCLUDE_DIR LIBRARIES)

if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  add_definitions(-DHAVE_BOOST)
endif()

if(Boost_date_time_FOUND)
  add_definitions(-DHAVE_BOOST_DATE_TIME)
endif()
