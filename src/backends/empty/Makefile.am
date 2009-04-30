# $Id: Makefile.am,v 1.5 2008/04/28 19:58:54 mloskot Exp $
# 
# Copyright (C) 2006, Mateusz Loskot
#
# Author: Mateusz Loskot <mateusz@loskot.net>
#
INCLUDES = -I$(top_srcdir)/core
AM_CXXFLAGS = -fPIC

noinst_LTLIBRARIES	= \
	lib@SOCI_EMPTY_LIBRARY_NAME@.la

lib@SOCI_EMPTY_LIBRARY_NAME@_la_LDFLAGS = \
	-release @LIBTOOL_RELEASE@ -no-undefined

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

soci_emptydir = $(includedir)/soci/empty

soci_empty_HEADERS = \
	soci-empty.h

EXTRA_DIST = Makefile.basic

MAINTAINERCLEANLOCAL = Makefile.in

maintainer-clean-local:
	-test -z "$(MAINTAINERCLEANLOCAL)" || rm -rf $(MAINTAINERCLEANLOCAL)
