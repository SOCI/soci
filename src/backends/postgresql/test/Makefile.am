# $Id: Makefile.am,v 1.6 2008/04/28 19:58:55 mloskot Exp $
# 
# Author: Mateusz Loskot <mateusz@loskot.net>
#
AM_CPPFLAGS = @BOOST_CPPFLAGS@ @POSTGRESQL_CFLAGS@
AM_LDFLAGS = @POSTGRESQL_LDFLAGS@

INCLUDES = \
	-I$(top_srcdir)/core \
	-I$(top_srcdir)/core/test \
	-I$(top_srcdir)/backends/postgresql

noinst_PROGRAMS = \
	test_postgresql

test_postgresql_SOURCES = \
	test-postgresql.cpp

test_postgresql_LDADD = \
	$(top_builddir)/core/lib@SOCI_CORE_LIBRARY_NAME@.la \
	$(top_builddir)/backends/postgresql/lib@SOCI_POSTGRESQL_LIBRARY_NAME@.la

EXTRA_DIST = Makefile.basic

MAINTAINERCLEANLOCAL = Makefile.in

maintainer-clean-local:
	-test -z "$(MAINTAINERCLEANLOCAL)" || rm -rf $(MAINTAINERCLEANLOCAL)
