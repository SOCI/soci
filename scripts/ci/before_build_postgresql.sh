#!/usr/bin/env bash
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
        set -x
        whoami
        cd /usr/local/var
        ls -l postgresql@14
        ln -s postgresql@14 postgres
        pg_ctl start
        pg_isready --timeout=60
        createuser --superuser postgres
        set +x
        ;;

    *)
        echo 'Unknown OS, PostgreSQL not available'
        exit 1
esac

psql --version
psql -c 'create database soci_test;' -U postgres

echo 'Testing connection to the database:'
psql -c 'select user;' -U postgres -d soci_test
