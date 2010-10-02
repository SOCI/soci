set(Doxygen_FIND_QUIETLY TRUE)

find_package(Doxygen)

boost_external_report(Doxygen EXECUTABLE DOT_EXECUTABLE)