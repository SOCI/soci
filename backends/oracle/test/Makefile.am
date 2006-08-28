# $Id: Makefile.am,v 1.2 2006/08/28 03:07:57 stephenhutton Exp $
# 
# Author: Mateusz Loskot <mateusz@loskot.net>
#
INCLUDES = \
	-I$(top_srcdir)/src/core \
	-I$(top_srcdir)/src/core/test \
	-I$(top_srcdir)/src/backends/oracle

AM_CPPFLAGS = @ORACLE_OCI_CFLAGS@
AM_LDFLAGS = @ORACLE_OCI_LDFLAGS@

noinst_PROGRAMS = \
	test_oracle

test_oracle_SOURCES = \
	test-oracle.cpp

test_oracle_LDADD = \
	$(top_srcdir)/src/core/libsoci_core.la \
	$(top_srcdir)/src/backends/oracle/libsoci_oracle.la

MAINTAINERCLEANLOCAL = Makefile.in

maintainer-clean-local:
	-test -z "$(MAINTAINERCLEANLOCAL)" || rm -rf $(MAINTAINERCLEANLOCAL)
