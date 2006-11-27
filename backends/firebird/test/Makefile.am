# $Id: Makefile.am,v 1.4 2006/11/27 09:59:31 mloskot Exp $
# 
# Author: Mateusz Loskot <mateusz@loskot.net>
#
INCLUDES = \
	-I$(top_srcdir)/src/core \
	-I$(top_srcdir)/src/core/test \
	-I$(top_srcdir)/src/backends/firebird

AM_CPPFLAGS = @FIREBIRD_CFLAGS@
AM_LDFLAGS = @FIREBIRD_LDFLAGS@

noinst_PROGRAMS = \
	test_firebird

test_firebird_SOURCES = \
	test-firebird.cpp

test_firebird_LDADD = \
	$(top_srcdir)/src/core/lib@SOCI_CORE_LIBRARY_NAME@.la \
	$(top_srcdir)/src/backends/firebird/lib@SOCI_FIREBIRD_LIBRARY_NAME@.la

EXTRA_DIST = Makefile.basic

MAINTAINERCLEANLOCAL = Makefile.in

maintainer-clean-local:
	-test -z "$(MAINTAINERCLEANLOCAL)" || rm -rf $(MAINTAINERCLEANLOCAL)
