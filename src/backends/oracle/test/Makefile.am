# $Id: Makefile.am,v 1.6 2008/04/28 19:58:54 mloskot Exp $
# 
# Author: Mateusz Loskot <mateusz@loskot.net>
#
AM_CPPFLAGS = @BOOST_CPPFLAGS@ @ORACLE_OCI_CFLAGS@
AM_LDFLAGS = @ORACLE_OCI_LDFLAGS@

INCLUDES = \
	-I$(top_srcdir)/core \
	-I$(top_srcdir)/core/test \
	-I$(top_srcdir)/backends/oracle

noinst_PROGRAMS = \
	test_oracle

test_oracle_SOURCES = \
	test-oracle.cpp

test_oracle_LDADD = \
	$(top_builddir)/core/lib@SOCI_CORE_LIBRARY_NAME@.la \
	$(top_builddir)/backends/oracle/lib@SOCI_ORACLE_LIBRARY_NAME@.la

EXTRA_DIST = Makefile.basic

MAINTAINERCLEANLOCAL = Makefile.in

maintainer-clean-local:
	-test -z "$(MAINTAINERCLEANLOCAL)" || rm -rf $(MAINTAINERCLEANLOCAL)
