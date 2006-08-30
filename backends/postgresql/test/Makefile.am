# $Id: Makefile.am,v 1.3 2006/08/30 14:25:04 mloskot Exp $
# 
# Author: Mateusz Loskot <mateusz@loskot.net>
#
INCLUDES = \
	-I$(top_srcdir)/src/core \
	-I$(top_srcdir)/src/core/test \
	-I$(top_srcdir)/src/backends/postgresql

AM_CPPFLAGS = @POSTGRESQL_CFLAGS@
AM_LDFLAGS = @POSTGRESQL_LDFLAGS@

noinst_PROGRAMS = \
	test_postgresql

test_postgresql_SOURCES = \
	test-postgresql.cpp

test_postgresql_LDADD = \
	$(top_srcdir)/src/core/lib@SOCI_CORE_LIBRARY_NAME@.la \
	$(top_srcdir)/src/backends/postgresql/lib@SOCI_POSTGRESQL_LIBRARY_NAME@.la

MAINTAINERCLEANLOCAL = Makefile.in

maintainer-clean-local:
	-test -z "$(MAINTAINERCLEANLOCAL)" || rm -rf $(MAINTAINERCLEANLOCAL)
