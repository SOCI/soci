# $Id: Makefile.am,v 1.2 2006/08/28 03:07:57 stephenhutton Exp $
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
	$(top_srcdir)/src/core/libsoci_core.la \
	$(top_srcdir)/src/backends/postgresql/libsoci_postgresql.la

MAINTAINERCLEANLOCAL = Makefile.in

maintainer-clean-local:
	-test -z "$(MAINTAINERCLEANLOCAL)" || rm -rf $(MAINTAINERCLEANLOCAL)
