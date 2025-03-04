#!/usr/bin/env bash
# Sets up environment for SOCI backend MySQL in CI builds
#
# Mateusz Loskot <mateusz@loskot.net>, https://github.com/SOCI
#
source ${SOCI_SOURCE_DIR}/scripts/ci/common.sh

SOCI_MYSQL_USER=$(id -un)

if [ -n "${SOCI_MYSQL_ROOT_PASSWORD}" ]; then
    if [[ "$SOCI_MYSQL_USE_MARIADB" = "YES" ]]; then
        sudo systemctl start mariadb.service
    else
        sudo systemctl start mysql.service
    fi

    SOCI_MYSQL_PASSWORD_OPT="-p${SOCI_MYSQL_ROOT_PASSWORD}"
    sudo mysql -u root ${SOCI_MYSQL_PASSWORD_OPT} -e "CREATE USER '${SOCI_MYSQL_USER}';"
fi

mysql --version
sudo mysql -u root ${SOCI_MYSQL_PASSWORD_OPT} -e "CREATE DATABASE soci_test;"
sudo mysql -u root ${SOCI_MYSQL_PASSWORD_OPT} -e "GRANT ALL PRIVILEGES ON soci_test.* TO '${SOCI_MYSQL_USER}'@'%';";

# This is necessary for MySQL stored procedure unit test to work.
sudo mysql -u root ${SOCI_MYSQL_PASSWORD_OPT} -e "SET GLOBAL log_bin_trust_function_creators=1";

echo 'Testing connection to the database:'
echo 'SELECT USER();' | mysql --database=soci_test
