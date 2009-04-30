# $Id: Makefile.am,v 1.5 2008/05/13 11:59:34 mloskot Exp $
# 
# Author: Mateusz Loskot <mateusz@loskot.net>
#
INCLUDES = -I$(top_srcdir)/core
AM_CXXFLAGS = -fPIC
AM_CPPFLAGS = @FIREBIRD_CFLAGS@

lib_LTLIBRARIES	= \
	lib@SOCI_FIREBIRD_LIBRARY_NAME@.la

lib@SOCI_FIREBIRD_LIBRARY_NAME@_la_LDFLAGS = \
	-release @LIBTOOL_RELEASE@ -no-undefined @FIREBIRD_LDFLAGS@

lib@SOCI_FIREBIRD_LIBRARY_NAME@_la_LIBADD = \
	@top_builddir@/core/lib@SOCI_CORE_LIBRARY_NAME@.la

lib@SOCI_FIREBIRD_LIBRARY_NAME@_la_SOURCES = \
	blob.cpp \
	common.cpp \
	error-firebird.cpp \
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
	error-firebird.h \
	soci-firebird.h

EXTRA_DIST = Makefile.basic

MAINTAINERCLEANLOCAL = Makefile.in

maintainer-clean-local:
	-test -z "$(MAINTAINERCLEANLOCAL)" || rm -rf $(MAINTAINERCLEANLOCAL)
