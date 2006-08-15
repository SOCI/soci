# $Id: Makefile.am,v 1.1 2006/08/15 14:02:18 mloskot Exp $
# 
# Author: Mateusz Loskot <mateusz@loskot.net>
#
INCLUDES = -I$(top_srcdir)/src/core
AM_CXXFLAGS = -fPIC
AM_CPPFLAGS = @MYSQL_CFLAGS@
AM_LDFLAGS = @MYSQL_LDFLAGS@

lib_LTLIBRARIES	= \
	libsoci_mysql.la

libsoci_mysql_la_LDFLAGS = \
	-release @LIBTOOL_RELEASE@

libsoci_mysql_la_SOURCES = \
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
