#!/bin/bash -e
# Sets up environment for SOCI backend MySQL at travis-ci.org
#
# Mateusz Loskot <mateusz@loskot.net>, http://github.com/SOCI
#
source ${TRAVIS_BUILD_DIR}/scripts/travis/common.sh

mysql --version
mysql -u root -e "CREATE DATABASE soci_test;"
mysql -u root -e "GRANT ALL PRIVILEGES ON soci_test.* TO 'travis'@'%';";
