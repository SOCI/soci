#!/bin/bash

source "${SOCI_SOURCE_DIR}/scripts/ci/common.sh"

if [[ "$SOCI_MYSQL_USE_MARIADB" = "YES" ]]; then
    run_apt purge mysql*

    run_apt install mariadb-server mariadb-client libmariadb-dev

    sudo systemctl start mariadb

    systemctl status mariadb
else
    # We assume that MySQL is already installed
    sudo systemctl start mysql
    systemctl status mysql
fi
