# $Id: Makefile.am,v 1.6 2008/05/12 10:56:32 mloskot Exp $
# 
# Copyright (C) 2006, Mateusz Loskot
#
# Author: Mateusz Loskot <mateusz@loskot.net>
#
AM_CPPFLAGS = $(BOOST_CPPFLAGS)

INCLUDES = \
	-I$(top_srcdir)/core \
	-I$(top_srcdir)/backends/empty

noinst_PROGRAMS = \
	test_empty

test_empty_SOURCES = \
	test-empty.cpp

test_empty_LDADD = \
	$(top_builddir)/core/lib@SOCI_CORE_LIBRARY_NAME@.la \
	$(top_builddir)/backends/empty/lib@SOCI_EMPTY_LIBRARY_NAME@.la

EXTRA_DIST = Makefile.basic

check-local: test_empty
	./test_empty dummystr

MAINTAINERCLEANLOCAL = Makefile.in

maintainer-clean-local:
	-test -z "$(MAINTAINERCLEANLOCAL)" || rm -rf $(MAINTAINERCLEANLOCAL)
