# $Id: Makefile.am,v 1.1 2006/08/15 13:51:00 mloskot Exp $
# 
# Author: Mateusz Loskot <mateusz@loskot.net>
#
INCLUDES = \
	-I$(top_srcdir)/src/core \
	-I$(top_srcdir)/src/backends/empty

noinst_PROGRAMS = \
	test_empty

test_empty_SOURCES = \
	test-empty.cpp

test_empty_LDADD = \
	$(top_srcdir)/src/core/libsoci_core.la \
	$(top_srcdir)/src/backends/empty/libsoci_empty.la

MAINTAINERCLEANLOCAL = Makefile.in

maintainer-clean-local:
	-test -z "$(MAINTAINERCLEANLOCAL)" || rm -rf $(MAINTAINERCLEANLOCAL)
