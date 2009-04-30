# $Id: Makefile.am,v 1.2 2008/05/12 13:21:25 mloskot Exp $
# 
# Author: Mateusz Loskot <mateusz@loskot.net>
#
INCLUDES = -I$(top_srcdir)/core
AM_CXXFLAGS = -fPIC
AM_CPPFLAGS = @ODBC_CFLAGS@

lib_LTLIBRARIES	= \
	lib@SOCI_ODBC_LIBRARY_NAME@.la

lib@SOCI_ODBC_LIBRARY_NAME@_la_LDFLAGS = \
	-release @LIBTOOL_RELEASE@ -no-undefined @ODBC_LDFLAGS@

lib@SOCI_ODBC_LIBRARY_NAME@_la_LIBADD = \
	@top_builddir@/core/lib@SOCI_CORE_LIBRARY_NAME@.la

lib@SOCI_ODBC_LIBRARY_NAME@_la_SOURCES = \
	blob.cpp \
	factory.cpp \
	row-id.cpp \
	session.cpp \
	standard-into-type.cpp \
	standard-use-type.cpp \
	statement.cpp \
	vector-into-type.cpp \
	vector-use-type.cpp

EXTRA_DIST = \
	Makefile.basic

soci_odbcdir = $(includedir)/soci/odbc

soci_odbc_HEADERS = \
	utility.h
	soci-odbc.h

MAINTAINERCLEANLOCAL = Makefile.in

maintainer-clean-local:
	-test -z "$(MAINTAINERCLEANLOCAL)" || rm -rf $(MAINTAINERCLEANLOCAL)
