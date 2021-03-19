#!/bin/bash -e
# Sets up environment for SOCI backend PostgreSQL in CI builds
#
# Mateusz Loskot <mateusz@loskot.net>, http://github.com/SOCI
#
source ${SOCI_SOURCE_DIR}/scripts/ci/common.sh

if [ $(uname) = Darwin ]; then
    pg_ctl start
    pg_isready --timeout=60
    createuser --superuser postgres
fi

psql --version
psql -c 'create database soci_test;' -U postgres
