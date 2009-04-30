# $Id: Makefile.am,v 1.4 2008/04/28 19:58:55 mloskot Exp $
# 
# Author: Mateusz Loskot <mateusz@loskot.net>
#
INCLUDES = -I$(top_srcdir)/core
AM_CXXFLAGS = -fPIC
AM_CPPFLAGS = @SQLITE3_CFLAGS@

lib_LTLIBRARIES	= \
	lib@SOCI_SQLITE3_LIBRARY_NAME@.la

lib@SOCI_SQLITE3_LIBRARY_NAME@_la_LDFLAGS = \
	-release @LIBTOOL_RELEASE@ -no-undefined @SQLITE3_LDFLAGS@

lib@SOCI_SQLITE3_LIBRARY_NAME@_la_LIBADD = \
	@top_builddir@/core/lib@SOCI_CORE_LIBRARY_NAME@.la

lib@SOCI_SQLITE3_LIBRARY_NAME@_la_SOURCES = \
	blob.cpp \
	common.cpp \
	factory.cpp \
	row-id.cpp \
	session.cpp \
	standard-into-type.cpp \
	standard-use-type.cpp \
	statement.cpp \
	vector-into-type.cpp \
	vector-use-type.cpp

soci_sqlite3dir = $(includedir)/soci/sqlite3

soci_sqlite3_HEADERS = \
	common.h \
	soci-sqlite3.h

EXTRA_DIST = Makefile.basic

MAINTAINERCLEANLOCAL = Makefile.in

maintainer-clean-local:
	-test -z "$(MAINTAINERCLEANLOCAL)" || rm -rf $(MAINTAINERCLEANLOCAL)
