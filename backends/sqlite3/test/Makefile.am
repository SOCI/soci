# $Id: Makefile.am,v 1.3 2006/09/23 21:35:51 mloskot Exp $
# 
# Author: Mateusz Loskot <mateusz@loskot.net>
#
INCLUDES = \
	-I$(top_srcdir)/src/core \
	-I$(top_srcdir)/src/core/test \
	-I$(top_srcdir)/src/backends/sqlite3

AM_CPPFLAGS = @SQLITE3_CFLAGS@
AM_LDFLAGS = @SQLITE3_LDFLAGS@

TEST_CASE = test_sqlite3
TEST_DB = test_sqlite3.db

noinst_PROGRAMS = \
	$(TEST_CASE)

#test_sqlite3_CPPFLAGS = \
#	-DSQLITE_ENABLE_COLUMN_METADATA

test_sqlite3_SOURCES = \
	test-sqlite3.cpp

test_sqlite3_LDADD = \
	$(top_srcdir)/src/core/lib@SOCI_CORE_LIBRARY_NAME@.la \
	$(top_srcdir)/src/backends/sqlite3/lib@SOCI_SQLITE3_LIBRARY_NAME@.la

check-local:
	$(SHELL) -x $(TEST_CASE) $(TEST_DB)

clean-local:
	$(RM) $(TEST_DB)
	
MAINTAINERCLEANLOCAL = Makefile.in

maintainer-clean-local:
	-test -z "$(MAINTAINERCLEANLOCAL)" || rm -rf $(MAINTAINERCLEANLOCAL)
