#!/bin/bash -e
# Sets up environment for SOCI backend MySQL in CI builds
#
# Mateusz Loskot <mateusz@loskot.net>, http://github.com/SOCI
#
source ${SOCI_SOURCE_DIR}/scripts/ci/common.sh

mysql --version
mysql -u root -e "CREATE DATABASE soci_test;"
mysql -u root -e "GRANT ALL PRIVILEGES ON soci_test.* TO 'travis'@'%';";
