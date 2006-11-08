# $Id: maketest.mak,v 1.1 2006/11/08 18:29:55 mloskot Exp $
#
# Makefile to run SOCI tests using NMAKE utility on Windows.
#
# CONFIGURATION
#
# Here, define connection settings for your environment
FIREBIRD_CONN="service=C:\Progra~1\Firebird\Firebird_1_5\data\SOCI_TEST.FDB user=sysdba password=buildbot"
MYSQL_CONN=""
ORACLE_CONN="service=localhost user=system password=buildbot"
POSTGRESQL_CONN="dbname=soci_test user=postgres password=buildbot"
SQLITE3_CONN=sqlite3_test.sdb
#
# Test programs
#
FIREBIRD_TEST = firebird_test.exe
MYSQL_TEST = mysql_test.exe
ORACLE_TEST = oracle_test.exe
POSTGRESQL_TEST = postgresql_test.exe
SQLITE3_TEST = sqlite3_test.exe
# 
# Release or Debug
#
BUILDCONFIG = debug
#
# END OF CONFIGURATION

RM = -del

all:	firebird oracle postgresql sqlite3
	#mysql 

default:	all

firebird:	$(BUILDCONFIG)/$(FIREBIRD_TEST)
	cd $(BUILDCONFIG)
	$(FIREBIRD_TEST) $(FIREBIRD_CONN)
	cd ..

mysql:	$(BUILDCONFIG)/$(MYSQL_TEST)

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
