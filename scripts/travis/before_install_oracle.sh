#!/bin/bash
# Script performs non-interactive installation of Oracle XE on Linux
#
# Uses Oracle downloader and installer from https://github.com/cbandy/travis-oracle
#
# set -ex
source ${TRAVIS_BUILD_DIR}/scripts/travis/oracle.sh

wget https://raw.githubusercontent.com/Vincit/travis-oracledb-xe/master/accept_the_license_agreement_for_oracledb_xe_11g_and_install.sh
bash ./accept_the_license_agreement_for_oracledb_xe_11g_and_install.sh
