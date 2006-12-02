# $Id: Makefile.am,v 1.3 2006/11/27 09:59:31 mloskot Exp $
# 
# Author: Mateusz Loskot <mateusz@loskot.net>
#
INCLUDES = \
	-I$(top_srcdir)/src/core \
	-I$(top_srcdir)/src/backends/mysql

AM_CPPFLAGS = @MYSQL_CFLAGS@
AM_LDFLAGS = @MYSQL_LDFLAGS@

noinst_PROGRAMS = \
	test_mysql

test_mysql_SOURCES = \
	test-mysql.cpp

test_mysql_LDADD = \
	$(top_srcdir)/src/core/lib@SOCI_CORE_LIBRARY_NAME@.la \
	$(top_srcdir)/src/backends/mysql/lib@SOCI_MYSQL_LIBRARY_NAME@.la

EXTRA_DIST = Makefile.basic

MAINTAINERCLEANLOCAL = Makefile.in

maintainer-clean-local:
	-test -z "$(MAINTAINERCLEANLOCAL)" || rm -rf $(MAINTAINERCLEANLOCAL)
