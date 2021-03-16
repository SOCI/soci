#!/bin/bash -e
# Sets up environment for SOCI backend PostgreSQL at travis-ci.org
#
# Mateusz Loskot <mateusz@loskot.net>, http://github.com/SOCI
#
source ${SOCI_SOURCE_DIR}/scripts/ci/common.sh

psql --version
psql -c 'create database soci_test;' -U postgres