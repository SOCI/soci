# $Id: Makefile.am,v 1.1 2006/08/15 13:56:24 mloskot Exp $
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
	$(top_srcdir)/src/core/libsoci_core.la \
	$(top_srcdir)/src/backends/firebird/libsoci_firebird.la

MAINTAINERCLEANLOCAL = Makefile.in

maintainer-clean-local:
	-test -z "$(MAINTAINERCLEANLOCAL)" || rm -rf $(MAINTAINERCLEANLOCAL)
