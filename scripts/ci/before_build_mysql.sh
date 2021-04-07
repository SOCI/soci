#!/bin/bash -e
# Sets up environment for SOCI backend MySQL in CI builds
#
# Mateusz Loskot <mateusz@loskot.net>, http://github.com/SOCI
#
source ${SOCI_SOURCE_DIR}/scripts/ci/common.sh

SOCI_MYSQL_USER=$(id -un)

if [ -n "${SOCI_MYSQL_ROOT_PASSWORD}" ]; then
    sudo systemctl start mysql.service
    SOCI_MYSQL_PASSWORD_OPT="-p${SOCI_MYSQL_ROOT_PASSWORD}"
    mysql -u root ${SOCI_MYSQL_PASSWORD_OPT} -e "CREATE USER '${SOCI_MYSQL_USER}';"
fi

mysql --version
mysql -u root ${SOCI_MYSQL_PASSWORD_OPT} -e "CREATE DATABASE soci_test;"
mysql -u root ${SOCI_MYSQL_PASSWORD_OPT} -e "GRANT ALL PRIVILEGES ON soci_test.* TO '${SOCI_MYSQL_USER}'@'%';";

echo 'Testing connection to the database:'
echo 'SELECT USER();' | mysql --database=soci_test
