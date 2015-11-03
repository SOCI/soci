#!/usr/bin/env bash
# Part of Vagrant virtual development environments for SOCI

# Installs DB2 Express-C 9.7
# Pre-installation
source /vagrant/bin/vagrant/common.env
export DEBIAN_FRONTEND="noninteractive"
# Installation
/vagrant/bin/ci/before_install_db2.sh
# Post-installation
sudo -u db2inst1 -i db2 "CREATE DATABASE ${SOCI_USER}"
sudo -u db2inst1 -i db2 "ACTIVATE DATABASE ${SOCI_USER}"
