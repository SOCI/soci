include(GNUInstallDirs)

set(SOCI_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/soci-${PROJECT_VERSION}"
  CACHE FILEPATH "Directory into which cmake related files (e.g. SOCIConfig.cmake) are installed")

set(SOCI_INSTALL_BINDIR "${CMAKE_INSTALL_BINDIR}"
  CACHE FILEPATH "Directory into which to install any SOCI executables")

set(SOCI_INSTALL_LIBDIR "${CMAKE_INSTALL_LIBDIR}"
  CACHE FILEPATH "Directory into which any SOCI libraries (except DLLs on Windows) are installed")

# Note that our headers are installed via a file set that already has the headers
# in a dedicated "soci" subdirectory (which will be part of the installation)
set(SOCI_INSTALL_INCLUDEDIR "${CMAKE_INSTALL_INCLUDEDIR}"
  CACHE FILEPATH "Directory into which the 'soci' directory with all SOCI header files is installed")
