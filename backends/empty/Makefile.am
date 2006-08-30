# $Id: Makefile.am,v 1.2 2006/08/30 14:25:03 mloskot Exp $
# 
# Copyright (C) 2006, Mateusz Loskot
#
# Author: Mateusz Loskot <mateusz@loskot.net>
#
INCLUDES = -I$(top_srcdir)/src/core
AM_CXXFLAGS = -fPIC

noinst_LTLIBRARIES	= \
	lib@SOCI_EMPTY_LIBRARY_NAME@.la

lib@SOCI_EMPTY_LIBRARY_NAME@_la_LDFLAGS = \
	-release @LIBTOOL_RELEASE@

lib@SOCI_EMPTY_LIBRARY_NAME@_la_SOURCES = \
	blob.cpp \
	factory.cpp \
	row-id.cpp \
	session.cpp \
	standard-into-type.cpp \
	standard-use-type.cpp \
	statement.cpp \
	vector-into-type.cpp \
	vector-use-type.cpp

MAINTAINERCLEANLOCAL = Makefile.in

maintainer-clean-local:
	-test -z "$(MAINTAINERCLEANLOCAL)" || rm -rf $(MAINTAINERCLEANLOCAL)
