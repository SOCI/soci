# $Id: Makefile.am,v 1.1 2006/08/15 13:56:24 mloskot Exp $
# 
# Author: Mateusz Loskot <mateusz@loskot.net>
#
INCLUDES = -I$(top_srcdir)/src/core
AM_CXXFLAGS = -fPIC
AM_CPPFLAGS = @FIREBIRD_CFLAGS@
AM_LDFLAGS = @FIREBIRD_LDFLAGS@

lib_LTLIBRARIES	= \
	libsoci_firebird.la

libsoci_firebird_la_LDFLAGS = \
	-release @LIBTOOL_RELEASE@

libsoci_firebird_la_SOURCES = \
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

#SUBDIRS = test

MAINTAINERCLEANLOCAL = Makefile.in

maintainer-clean-local:
	-test -z "$(MAINTAINERCLEANLOCAL)" || rm -rf $(MAINTAINERCLEANLOCAL)
