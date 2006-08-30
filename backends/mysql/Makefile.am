# $Id: Makefile.am,v 1.2 2006/08/30 14:25:04 mloskot Exp $
# 
# Author: Mateusz Loskot <mateusz@loskot.net>
#
INCLUDES = -I$(top_srcdir)/src/core
AM_CXXFLAGS = -fPIC
AM_CPPFLAGS = @MYSQL_CFLAGS@
AM_LDFLAGS = @MYSQL_LDFLAGS@

lib_LTLIBRARIES	= \
	lib@SOCI_MYSQL_LIBRARY_NAME@.la

lib@SOCI_MYSQL_LIBRARY_NAME@_la_LDFLAGS = \
	-release @LIBTOOL_RELEASE@

lib@SOCI_MYSQL_LIBRARY_NAME@_la_SOURCES = \
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

soci_mysqldir = $(includedir)/soci/mysql

soci_mysql_HEADERS = \
	common.h \
	soci-mysql.h

#SUBDIRS = test

MAINTAINERCLEANLOCAL = Makefile.in

maintainer-clean-local:
	-test -z "$(MAINTAINERCLEANLOCAL)" || rm -rf $(MAINTAINERCLEANLOCAL)
