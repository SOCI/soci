# $Id: Makefile.am,v 1.1 2006/08/15 13:34:25 mloskot Exp $
# 
# Author: Mateusz Loskot <mateusz@loskot.net>
#
AM_CXXFLAGS = -fPIC

lib_LTLIBRARIES	= libsoci_core.la

libsoci_core_la_LDFLAGS = \
	-release @LIBTOOL_RELEASE@

libsoci_core_la_SOURCES = \
	soci.cpp

socidir = $(includedir)/soci

soci_HEADERS = \
	soci.h \
	soci-backend.h

MAINTAINERCLEANLOCAL = Makefile.in

maintainer-clean-local:
	-test -z "$(MAINTAINERCLEANLOCAL)" || rm -rf $(MAINTAINERCLEANLOCAL)
