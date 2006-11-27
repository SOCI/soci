# $Id: Makefile.am,v 1.4 2006/11/27 09:59:32 mloskot Exp $
# 
# Copyright (C) 2006, Mateusz Loskot
#
# Author: Mateusz Loskot <mateusz@loskot.net>
#
AM_CXXFLAGS = -fPIC

SUBDIRS = test

lib_LTLIBRARIES	= lib@SOCI_CORE_LIBRARY_NAME@.la

lib@SOCI_CORE_LIBRARY_NAME@_la_LDFLAGS = \
	-release @LIBTOOL_RELEASE@

lib@SOCI_CORE_LIBRARY_NAME@_la_SOURCES = \
	soci.cpp

socidir = $(includedir)/soci

soci_HEADERS = \
	soci.h \
	soci-backend.h \
	soci-config.h \
	soci-platform.h

EXTRA_DIST = Makefile.basic

MAINTAINERCLEANLOCAL = Makefile.in

maintainer-clean-local:
	-test -z "$(MAINTAINERCLEANLOCAL)" || rm -rf $(MAINTAINERCLEANLOCAL)
