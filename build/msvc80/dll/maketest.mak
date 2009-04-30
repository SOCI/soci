# $Id: maketest.mak,v 1.3 2007/08/17 02:30:40 mloskot Exp $
#
# Makefile to run SOCI tests using NMAKE utility on Windows.
#
# Usage:
#
# 1. Run all tests
#
#    nmake /f maketest.mak
#
# 2. Run selected test
#
#    nmake /f maketest.mak <test-name>
#
#    ie. nmake /f maketest.mak mysql
#
######################################################################################
# CONFIGURATION
#
# Here, define connection settings for your environment
#
FIREBIRD_CONN="service=\\.\D:\dev\soci\data\SOCI_TEST.FDB user=sysdba password=masterkey"
MYSQL_CONN=""
ODBC_ACCESS_CONN="FILEDSN=D:\data\soci\test-access.dsn"
ODBC_MSSQL_CONN="FILEDSN=D:\data\soci\test-mssql.dsn"
ODBC_MYSQL_CONN="FILEDSN=D:\data\soci\test-mysql.dsn"
ODBC_POSTGRESQL_CONN="FILEDSN=D:\data\soci\test-postgresql.dsn"
ORACLE_CONN="service=localhost user=system password=buildbot"
POSTGRESQL_CONN="dbname=soci_test user=postgres password=buildbot"
SQLITE3_CONN=sqlite3_test.sdb
#
# END OF CONFIGURATION
######################################################################################
#
# Test programs
#
FIREBIRD_TEST = firebird_test.exe
MYSQL_TEST = mysql_test.exe
ODBC_ACCESS_TEST = odbc_test_access.exe
ODBC_MSSQL_TEST = odbc_test_mssql.exe
ODBC_MYSQL_TEST = odbc_test_mysql.exe
ODBC_POSTGRESQL_TEST = odbc_test_postgresql.exe
ORACLE_TEST = oracle_test.exe
POSTGRESQL_TEST = postgresql_test.exe
SQLITE3_TEST = sqlite3_test.exe
#
# Release or Debug
#
BUILDCONFIG = debug

RM = -del

all:	firebird odbc-access odbc-mysql odbc-mssql odbc-postgresql oracle postgresql sqlite3

default:	all

firebird:	$(BUILDCONFIG)/$(FIREBIRD_TEST)
	cd $(BUILDCONFIG)
	$(FIREBIRD_TEST) $(FIREBIRD_CONN)
	cd ..

mysql:	$(BUILDCONFIG)/$(MYSQL_TEST)

odbc-access:	$(BUILDCONFIG)/$(ODBC_ACCESS_TEST)
	cd $(BUILDCONFIG)
	$(ODBC_ACCESS_TEST) $(ODBC_ACCESS_CONN)
	cd ..

odbc-mysql:	$(BUILDCONFIG)/$(ODBC_MYSQL_TEST)
	cd $(BUILDCONFIG)
	$(ODBC_MYSQL_TEST) $(ODBC_MYSQL_CONN)
	cd ..

odbc-mssql:	$(BUILDCONFIG)/$(ODBC_MSSQL_TEST)
	cd $(BUILDCONFIG)
	$(ODBC_MSSQL_TEST) $(ODBC_MSSQL_CONN)
	cd ..

odbc-postgresql:	$(BUILDCONFIG)/$(ODBC_POSTGRESQL_TEST)
	cd $(BUILDCONFIG)
	$(ODBC_POSTGRESQL_TEST) $(ODBC_POSTGRESQL_CONN)
	cd ..

oracle:	$(BUILDCONFIG)/$(ORACLE_TEST)
	cd $(BUILDCONFIG)
	$(ORACLE_TEST) $(ORACLE_CONN)
	cd ..

postgresql:	$(BUILDCONFIG)/$(POSTGRESQL_TEST)
	cd $(BUILDCONFIG)
	$(POSTGRESQL_TEST) $(POSTGRESQL_CONN)
	cd ..

sqlite3:	$(BUILDCONFIG)/$(SQLITE3_TEST)
	cd $(BUILDCONFIG)
	$(SQLITE3_TEST) $(SQLITE3_CONN)
	$(RM) $(SQLITE3_CONN)
	cd ..

# EOF
