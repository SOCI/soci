#!/usr/bin/env bash
# Part of Vagrant virtual development environments for SOCI

# Installs DB2 Express-C 9.7
# Pre-installation
source /vagrant/bin/vagrant/common.env
export DEBIAN_FRONTEND="noninteractive"
# Installation
/vagrant/bin/ci/before_install_db2.sh
# Post-installation
## Let's be gentle to DB2 and try to not to recreate databases
echo "db2: checking if ${SOCI_USER} database exists"
sudo -u db2inst1 -i db2 "LIST DATABASE DIRECTORY" | grep ${SOCI_USER}
if [ $? -eq 0 ]; then
  echo "db2: creating database ${SOCI_USER}"
  sudo -u db2inst1 -i db2 "CREATE DATABASE ${SOCI_USER}"
  sudo -u db2inst1 -i db2 "ACTIVATE DATABASE ${SOCI_USER}"
else
  echo "db2: database ${SOCI_USER} (may) exists, skipping"
fi
