# $Id: Makefile.am,v 1.2 2006/08/30 14:25:04 mloskot Exp $
# 
# Author: Mateusz Loskot <mateusz@loskot.net>
#
INCLUDES = -I$(top_srcdir)/src/core
AM_CXXFLAGS = -fPIC
AM_CPPFLAGS = @SQLITE3_CFLAGS@
AM_LDFLAGS = @SQLITE3_LDFLAGS@

lib_LTLIBRARIES	= \
	lib@SOCI_SQLITE3_LIBRARY_NAME@.la

lib@SOCI_SQLITE3_LIBRARY_NAME@_la_LDFLAGS = \
	-release @LIBTOOL_RELEASE@

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

#SUBDIRS = test

MAINTAINERCLEANLOCAL = Makefile.in

maintainer-clean-local:
	-test -z "$(MAINTAINERCLEANLOCAL)" || rm -rf $(MAINTAINERCLEANLOCAL)
