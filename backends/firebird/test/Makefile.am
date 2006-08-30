# $Id: Makefile.am,v 1.2 2006/08/30 14:25:03 mloskot Exp $
# 
# Author: Mateusz Loskot <mateusz@loskot.net>
#
INCLUDES = \
	-I$(top_srcdir)/src/core \
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

MAINTAINERCLEANLOCAL = Makefile.in

maintainer-clean-local:
	-test -z "$(MAINTAINERCLEANLOCAL)" || rm -rf $(MAINTAINERCLEANLOCAL)
