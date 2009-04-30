# $Id: Makefile.am,v 1.5 2008/04/28 19:58:54 mloskot Exp $
# 
# Author: Mateusz Loskot <mateusz@loskot.net>
#
AM_CPPFLAGS = @BOOST_CPPFLAGS@ @MYSQL_CFLAGS@
AM_LDFLAGS = @MYSQL_LDFLAGS@

INCLUDES = \
	-I$(top_srcdir)/core \
	-I$(top_srcdir)/backends/mysql

noinst_PROGRAMS = \
	test_mysql

test_mysql_SOURCES = \
	test-mysql.cpp

test_mysql_LDADD = \
	$(top_builddir)/core/lib@SOCI_CORE_LIBRARY_NAME@.la \
	$(top_builddir)/backends/mysql/lib@SOCI_MYSQL_LIBRARY_NAME@.la

EXTRA_DIST = Makefile.basic

MAINTAINERCLEANLOCAL = Makefile.in

maintainer-clean-local:
	-test -z "$(MAINTAINERCLEANLOCAL)" || rm -rf $(MAINTAINERCLEANLOCAL)
