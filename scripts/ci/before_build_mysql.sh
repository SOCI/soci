#!/bin/bash -e
# Sets up environment for SOCI backend MySQL in CI builds
#
# Mateusz Loskot <mateusz@loskot.net>, http://github.com/SOCI
#
source ${SOCI_SOURCE_DIR}/scripts/ci/common.sh

SOCI_MYSQL_USER=$(id -un)
SOCI_MYSQL_PASS='mypass'

if [ -n "${SOCI_MYSQL_ROOT_PASSWORD}" ]; then
    sudo systemctl start mysql.service
    SOCI_MYSQL_PASSWORD_OPT="-p${SOCI_MYSQL_ROOT_PASSWORD}"
    mysql -u root ${SOCI_MYSQL_PASSWORD_OPT} -e "CREATE USER '${SOCI_MYSQL_USER}' IDENTIFIED BY '${SOCI_MYSQL_PASS}';"
    if [ ! -f $HOME/.my.cnf ]; then
        cat > $HOME/.my.cnf <<EOF
[mysql]
user=${SOCI_MYSQL_USER}
password=${SOCI_MYSQL_PASS}
EOF
        echo Created $HOME/.my.cnf file with password for "${SOCI_MYSQL_USER}":
    else
        echo Using existing $HOME/.my.cnf file:
    fi
    echo '---------------------------------- >8 --------------------------------------'
    cat $HOME/.my.cnf | tr 'A-Za-z' 'N-ZA-Mn-za-m'
    echo '---------------------------------- >8 --------------------------------------'
    echo '(using ROT-13 to ensure the password is not stripped from the output)'
fi

mysql --version
mysql -u root ${SOCI_MYSQL_PASSWORD_OPT} -e "CREATE DATABASE soci_test;"
mysql -u root ${SOCI_MYSQL_PASSWORD_OPT} -e "GRANT ALL PRIVILEGES ON soci_test.* TO '${SOCI_MYSQL_USER}'@'%';";
