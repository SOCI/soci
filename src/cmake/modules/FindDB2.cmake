#TODO: Add detection on win32

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
  set(DB2_LIBDIR "lib")
else()
  set(DB2_LIBDIR "lib64")
endif()


find_path(DB2_INCLUDE_DIR sqlcli1.h
  $ENV{DB2_INCLUDE_DIR}
  $ENV{DB2_DIR}/include
  /opt/ibm/db2/V10.1/include
  /opt/ibm/db2/V9.7/include
  /opt/ibm/db2/V9.5/include
  /opt/ibm/db2/V9.1/include)

find_library(DB2_LIBRARY NAMES db2
    PATHS
    $ENV{DB2_LIB_DIR}
    $ENV{DB2_DIR}/${DB2_LIBDIR}
  /opt/ibm/db2/V10.1/${DB2_LIBDIR}
  /opt/ibm/db2/V9.7/${DB2_LIBDIR}
  /opt/ibm/db2/V9.5/${DB2_LIBDIR}
  /opt/ibm/db2/V9.1/${DB2_LIBDIR})

if(DB2_LIBRARY)
  get_filename_component(DB2_LIBRARY_DIR ${DB2_LIBRARY} PATH)
endif()

if(DB2_INCLUDE_DIR AND DB2_LIBRARY_DIR)
  set(DB2_FOUND TRUE)

  include_directories(${DB2_INCLUDE_DIR})
  link_directories(${DB2_LIBRARY_DIR})

endif()

set(DB2_LIBRARIES ${DB2_LIBRARY})

# Handle the QUIETLY and REQUIRED arguments and set DB2_FOUND to TRUE
# if all listed variables are TRUE
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DB2
  DEFAULT_MSG
  DB2_INCLUDE_DIR
  DB2_LIBRARIES)

mark_as_advanced(DB2_INCLUDE_DIR DB2_LIBRARIES)
