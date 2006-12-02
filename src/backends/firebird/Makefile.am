# $Id: Makefile.am,v 1.3 2006/11/27 09:59:31 mloskot Exp $
# 
# Author: Mateusz Loskot <mateusz@loskot.net>
#
INCLUDES = -I$(top_srcdir)/src/core
AM_CXXFLAGS = -fPIC
AM_CPPFLAGS = @FIREBIRD_CFLAGS@
AM_LDFLAGS = @FIREBIRD_LDFLAGS@

lib_LTLIBRARIES	= \
	lib@SOCI_FIREBIRD_LIBRARY_NAME@.la

lib@SOCI_FIREBIRD_LIBRARY_NAME@_la_LDFLAGS = \
	-release @LIBTOOL_RELEASE@

lib@SOCI_FIREBIRD_LIBRARY_NAME@_la_SOURCES = \
	blob.cpp \
	common.cpp \
	error.cpp \
	factory.cpp \
	row-id.cpp \
	session.cpp \
	standard-into-type.cpp \
	standard-use-type.cpp \
	statement.cpp \
	vector-into-type.cpp \
	vector-use-type.cpp

soci_firebirddir = $(includedir)/soci/firebird

soci_firebird_HEADERS = \
	common.h \
	error.h \
	soci-firebird.h

EXTRA_DIST = Makefile.basic

MAINTAINERCLEANLOCAL = Makefile.in

maintainer-clean-local:
	-test -z "$(MAINTAINERCLEANLOCAL)" || rm -rf $(MAINTAINERCLEANLOCAL)
