# $Id: Makefile.am,v 1.1 2006/08/15 14:05:53 mloskot Exp $
# 
# Author: Mateusz Loskot <mateusz@loskot.net>
#
INCLUDES = -I$(top_srcdir)/src/core
AM_CXXFLAGS = -fPIC
AM_CPPFLAGS = @ORACLE_OCI_CFLAGS@
AM_LDFLAGS = @ORACLE_OCI_LDFLAGS@

lib_LTLIBRARIES	= \
	libsoci_oracle.la

libsoci_oracle_la_LDFLAGS = \
	-release @LIBTOOL_RELEASE@

libsoci_oracle_la_SOURCES = \
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

#SUBDIRS = test

MAINTAINERCLEANLOCAL = Makefile.in

maintainer-clean-local:
	-test -z "$(MAINTAINERCLEANLOCAL)" || rm -rf $(MAINTAINERCLEANLOCAL)
