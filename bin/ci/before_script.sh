#!/bin/sh
# Run before_script actions for SOCI build at travis-ci.org
# Mateusz Loskot <mateusz@loskot.net>, http://github.com/SOCI
#
# Create local databases
# MySQL (service provided)
mysql --version
mysql -e 'create database soci_test;'
# PostgreSQL (service provided)
psql --version  
psql -c 'create database soci_test;' -U postgres
# Firebird (installed manually, see before_install.sh)
isql-fb -z -q -i /dev/null # --version
echo 'CREATE DATABASE "LOCALHOST:/tmp/soci_test.fdb" PAGE_SIZE = 16384;' > /tmp/create_soci_test.sql
isql-fb -u SYSDBA -p masterkey -i /tmp/create_soci_test.sql -q
cat /tmp/create_soci_test.sql