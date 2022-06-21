#!/usr/bin/env bash
# Configures Oracle database for SOCI Oracle backend CI builds.
#
# Copyright (c) 2013 Peter Butkovic <butkovic@gmail.com>
#
# Modified by Mateusz Loskot <mateusz@loskot.net>
# Changes:
# - Check connection as user for testing
#
# Copyright (c) 2021 Vadim Zeitlin <vz-soci@zeitlins.org>
# - Rewrote to work with Docker Oracle container

source ${SOCI_SOURCE_DIR}/scripts/ci/common.sh

# create the user for the tests
oracle_sqlplus sys/oracle AS SYSDBA <<SQL
create user travis identified by travis;
grant connect, resource to travis;
grant execute on sys.dbms_lock to travis;
grant create session, alter any procedure to travis;
SQL

# increase default=40 value of processes to prevent ORA-12520 failures while testing
echo "alter system set processes=300 scope=spfile;" | \
oracle_sqlplus sys/oracle AS SYSDBA

# restart the database for the parameter change above to be taken into account
echo "Restarting the database..."
oracle_exec /etc/init.d/oracle-xe restart

# confirm the parameter modification was taken into account
echo "show parameter processes;" | \
oracle_sqlplus sys/oracle AS SYSDBA

# check connection as user for testing
echo "Connecting using travis/travis@XE"
echo "SELECT * FROM product_component_version;" | \
oracle_sqlplus travis/travis@XE
