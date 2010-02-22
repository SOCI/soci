set(SOCI_BACKEND_NAME soci_empty)

file(GLOB HEADERS *.h)
file(GLOB SOURCES *.cpp)

###############################################################################
# core build targets

add_library(${SOCI_BACKEND_NAME} SHARED ${SOURCES})

target_link_libraries(${SOCI_BACKEND_NAME}
  ${Boost_date_time_LIBRARY})

###############################################################################
# core installation

# TODO
