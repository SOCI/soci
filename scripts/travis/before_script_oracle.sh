#!/bin/bash
# Sets up environment for Oracle testing at travis-ci.org
#
# Copyright (c) 2013 Peter Butkovic <butkovic@gmail.com>
#
# Copyright (c) 2015,2019 Mateusz Loskot <mateusz@loskot.net>
#
# Copyright (c) 2020 Vadim Zeitlin <vz-soci@zeitlins.org>
#
source ${TRAVIS_BUILD_DIR}/scripts/travis/oracle.sh

# Create a user for testing.
docker exec $ORACLE_CONT sh -c "sqlplus -S -L sys/$ORACLE_PWD AS SYSDBA <<EOF
ALTER SESSION SET \"_ORACLE_SCRIPT\"=true;
CREATE USER travis IDENTIFIED BY travis;
GRANT CONNECT, RESOURCE TO travis;
ALTER USER travis QUOTA UNLIMITED ON users;
EOF"

# check connection as user for testing
echo "Connecting using travis/travis@XE"
docker exec $ORACLE_CONT sh -c "sqlplus -S -L travis/travis@XE <<EOF
SELECT * FROM product_component_version;
EOF"
