# $Id: Makefile.am,v 1.6 2008/04/28 19:58:54 mloskot Exp $
# 
# Author: Mateusz Loskot <mateusz@loskot.net>
#
AM_CPPFLAGS = @BOOST_CPPFLAGS@ @FIREBIRD_CFLAGS@
AM_LDFLAGS = @FIREBIRD_LDFLAGS@

INCLUDES = \
	-I$(top_srcdir)/core \
	-I$(top_srcdir)/core/test \
	-I$(top_srcdir)/backends/firebird

noinst_PROGRAMS = \
	test_firebird

test_firebird_SOURCES = \
	test-firebird.cpp

test_firebird_LDADD = \
	$(top_builddir)/core/lib@SOCI_CORE_LIBRARY_NAME@.la \
	$(top_builddir)/backends/firebird/lib@SOCI_FIREBIRD_LIBRARY_NAME@.la

EXTRA_DIST = Makefile.basic

MAINTAINERCLEANLOCAL = Makefile.in

maintainer-clean-local:
	-test -z "$(MAINTAINERCLEANLOCAL)" || rm -rf $(MAINTAINERCLEANLOCAL)
