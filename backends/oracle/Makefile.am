# $Id: Makefile.am,v 1.3 2006/11/27 09:59:31 mloskot Exp $
# 
# Author: Mateusz Loskot <mateusz@loskot.net>
#
INCLUDES = -I$(top_srcdir)/src/core
AM_CXXFLAGS = -fPIC
AM_CPPFLAGS = @ORACLE_OCI_CFLAGS@
AM_LDFLAGS = @ORACLE_OCI_LDFLAGS@

lib_LTLIBRARIES	= \
	lib@SOCI_ORACLE_LIBRARY_NAME@.la

lib@SOCI_ORACLE_LIBRARY_NAME@_la_LDFLAGS = \
	-release @LIBTOOL_RELEASE@

lib@SOCI_ORACLE_LIBRARY_NAME@_la_SOURCES = \
	blob.cpp \
	error.cpp \
	factory.cpp \
	row-id.cpp \
	session.cpp \
	standard-into-type.cpp \
	standard-use-type.cpp \
	statement.cpp \
	vector-into-type.cpp \
	vector-use-type.cpp

soci_oracledir = $(includedir)/soci/oracle

soci_oracle_HEADERS = \
	error.h \
	soci-oracle.h

EXTRA_DIST = Makefile.basic

MAINTAINERCLEANLOCAL = Makefile.in

maintainer-clean-local:
	-test -z "$(MAINTAINERCLEANLOCAL)" || rm -rf $(MAINTAINERCLEANLOCAL)
