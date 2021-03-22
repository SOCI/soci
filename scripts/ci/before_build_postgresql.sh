#!/bin/bash -e
# Sets up environment for SOCI backend PostgreSQL in CI builds
#
# Mateusz Loskot <mateusz@loskot.net>, http://github.com/SOCI
#
source ${SOCI_SOURCE_DIR}/scripts/ci/common.sh

case "$(uname)" in
    Linux)
        sudo service postgresql start
        pg_isready --timeout=60
        sudo -u postgres psql -c "ALTER USER postgres PASSWORD 'Password12!';"
        ;;

    Darwin)
        pg_ctl start
        pg_isready --timeout=60
        createuser --superuser postgres
        ;;

    *)
        echo 'Unknown OS, PostgreSQL not available'
        exit 1
esac

psql --version
psql -c 'create database soci_test;' -U postgres

echo 'Testing connection to the database:'
psql -c 'select user;' -U postgres -d soci_test
