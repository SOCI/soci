# $Id: Makefile.am,v 1.1 2006/08/15 14:10:07 mloskot Exp $
# 
# Author: Mateusz Loskot <mateusz@loskot.net>
#
INCLUDES = -I$(top_srcdir)/src/core
AM_CXXFLAGS = -fPIC
AM_CPPFLAGS = @POSTGRESQL_CFLAGS@
AM_LDFLAGS = @POSTGRESQL_LDFLAGS@

lib_LTLIBRARIES	= \
	libsoci_postgresql.la

libsoci_postgresql_la_LDFLAGS = \
	-release @LIBTOOL_RELEASE@

libsoci_postgresql_la_SOURCES = \
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
