#!/bin/bash -e
# Sets up environment for SOCI backend MySQL at travis-ci.org
#
# Mateusz Loskot <mateusz@loskot.net>, http://github.com/SOCI
#
source ./bin/ci/common.sh
mysql --version
mysql -e 'create database soci_test;'
