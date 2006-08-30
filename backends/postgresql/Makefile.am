# $Id: Makefile.am,v 1.2 2006/08/30 14:25:04 mloskot Exp $
# 
# Author: Mateusz Loskot <mateusz@loskot.net>
#
INCLUDES = -I$(top_srcdir)/src/core
AM_CXXFLAGS = -fPIC
AM_CPPFLAGS = @POSTGRESQL_CFLAGS@
AM_LDFLAGS = @POSTGRESQL_LDFLAGS@

lib_LTLIBRARIES	= \
	lib@SOCI_POSTGRESQL_LIBRARY_NAME@.la

lib@SOCI_POSTGRESQL_LIBRARY_NAME@_la_LDFLAGS = \
	-release @LIBTOOL_RELEASE@

lib@SOCI_POSTGRESQL_LIBRARY_NAME@_la_SOURCES = \
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

soci_postgresqldir = $(includedir)/soci/postgresql

soci_postgresql_HEADERS = \
	common.h \
	soci-postgresql.h

#SUBDIRS = test

MAINTAINERCLEANLOCAL = Makefile.in

maintainer-clean-local:
	-test -z "$(MAINTAINERCLEANLOCAL)" || rm -rf $(MAINTAINERCLEANLOCAL)
