#!/bin/bash -e
# Sets up environment for SOCI backend at travis-ci.org
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
# Copyright (c) 2015 Sergei Nikulov <sergey.nikulov@gmail.com>
#
source ${TRAVIS_BUILD_DIR}/scripts/travis/common.sh

mysql --version
mysql -u root -e "CREATE DATABASE soci_test;"
mysql -u root -e "GRANT ALL PRIVILEGES ON soci_test.* TO 'travis'@'%';";
psql --version
psql -c 'create database soci_test;' -U postgres
