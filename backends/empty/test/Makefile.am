# $Id: Makefile.am,v 1.5 2008/04/28 19:58:54 mloskot Exp $
# 
# Copyright (C) 2006, Mateusz Loskot
#
# Author: Mateusz Loskot <mateusz@loskot.net>
#
AM_CPPFLAGS = $(BOOST_CPPFLAGS)

INCLUDES = \
	-I$(top_srcdir)/src/core \
	-I$(top_srcdir)/src/backends/empty

noinst_PROGRAMS = \
	test_empty

test_empty_SOURCES = \
	test-empty.cpp

test_empty_LDADD = \
	$(top_builddir)/src/core/lib@SOCI_CORE_LIBRARY_NAME@.la \
	$(top_builddir)/src/backends/empty/lib@SOCI_EMPTY_LIBRARY_NAME@.la

EXTRA_DIST = Makefile.basic

MAINTAINERCLEANLOCAL = Makefile.in

maintainer-clean-local:
	-test -z "$(MAINTAINERCLEANLOCAL)" || rm -rf $(MAINTAINERCLEANLOCAL)
