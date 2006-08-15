# $Id: Makefile.am,v 1.1 2006/08/15 13:51:00 mloskot Exp $
# 
# Author: Mateusz Loskot <mateusz@loskot.net>
#
INCLUDES = -I$(top_srcdir)/src/core
AM_CXXFLAGS = -fPIC

noinst_LTLIBRARIES	= \
	libsoci_empty.la

libsoci_empty_la_LDFLAGS = \
	-release @LIBTOOL_RELEASE@

libsoci_empty_la_SOURCES = \
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
