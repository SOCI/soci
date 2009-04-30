# $Id: Makefile.am,v 1.2 2008/05/12 13:21:25 mloskot Exp $
# 
# Author: Mateusz Loskot <mateusz@loskot.net>
#
AM_CPPFLAGS = @BOOST_CPPFLAGS@ @ODBC_CFLAGS@
AM_LDFLAGS = @ODBC_LDFLAGS@

INCLUDES = \
	-I$(top_srcdir)/core \
	-I$(top_srcdir)/core/test \
	-I$(top_srcdir)/backends/odbc

noinst_PROGRAMS = \
	test_odbc_access \
	test_odbc_mssql \
	test_odbc_mysql \
	test_odbc_postgresql

test_odbc_access_SOURCES = \
	test-odbc-access.cpp

test_odbc_mssql_SOURCES = \
	test-odbc-mssql.cpp

test_odbc_mysql_SOURCES = \
	test-odbc-mysql.cpp

test_odbc_postgresql_SOURCES = \
	test-odbc-postgresql.cpp

test_odbc_access_LDADD = \
	$(top_builddir)/core/lib@SOCI_CORE_LIBRARY_NAME@.la \
	$(top_builddir)/backends/odbc/lib@SOCI_ODBC_LIBRARY_NAME@.la

test_odbc_mssql_LDADD = \
	$(top_builddir)/core/lib@SOCI_CORE_LIBRARY_NAME@.la \
	$(top_builddir)/backends/odbc/lib@SOCI_ODBC_LIBRARY_NAME@.la

test_odbc_mysql_LDADD = \
	$(top_builddir)/core/lib@SOCI_CORE_LIBRARY_NAME@.la \
	$(top_builddir)/backends/odbc/lib@SOCI_ODBC_LIBRARY_NAME@.la

test_odbc_postgresql_LDADD = \
	$(top_builddir)/core/lib@SOCI_CORE_LIBRARY_NAME@.la \
	$(top_builddir)/backends/odbc/lib@SOCI_ODBC_LIBRARY_NAME@.la

EXTRA_DIST = \
	Makefile.basic \
	test-access.dsn \
	test-mssql.dsn \
	test-mysql.dsn \
	test-postgresql.dsn

MAINTAINERCLEANLOCAL = Makefile.in

maintainer-clean-local:
	-test -z "$(MAINTAINERCLEANLOCAL)" || rm -rf $(MAINTAINERCLEANLOCAL)
