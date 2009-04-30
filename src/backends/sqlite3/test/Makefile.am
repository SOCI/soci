# $Id: Makefile.am,v 1.6 2008/04/28 19:58:55 mloskot Exp $
# 
# Author: Mateusz Loskot <mateusz@loskot.net>
#
AM_CPPFLAGS = @BOOST_CPPFLAGS@ @SQLITE3_CFLAGS@
AM_LDFLAGS = @SQLITE3_LDFLAGS@

INCLUDES = \
	-I$(top_srcdir)/core \
	-I$(top_srcdir)/core/test \
	-I$(top_srcdir)/backends/sqlite3

TEST_CASE = test_sqlite3
TEST_DB = test_sqlite3.db

noinst_PROGRAMS = \
	$(TEST_CASE)

#test_sqlite3_CPPFLAGS = \
#	-DSQLITE_ENABLE_COLUMN_METADATA

test_sqlite3_SOURCES = \
	test-sqlite3.cpp

test_sqlite3_LDADD = \
	$(top_builddir)/core/lib@SOCI_CORE_LIBRARY_NAME@.la \
	$(top_builddir)/backends/sqlite3/lib@SOCI_SQLITE3_LIBRARY_NAME@.la

EXTRA_DIST = Makefile.basic

check-local:
	$(SHELL) -x $(TEST_CASE) $(TEST_DB)

clean-local:
	$(RM) $(TEST_DB)
	
MAINTAINERCLEANLOCAL = Makefile.in

maintainer-clean-local:
	-test -z "$(MAINTAINERCLEANLOCAL)" || rm -rf $(MAINTAINERCLEANLOCAL)
