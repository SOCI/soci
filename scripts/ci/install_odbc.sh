#!/bin/bash -e
# Install ODBC libraries for SOCI in CI builds
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
source ${SOCI_SOURCE_DIR}/scripts/ci/common.sh

sudo apt-get install -qq \
    tar bzip2 \
    unixodbc unixodbc-dev \
    odbc-postgresql odbc-postgresql-dbgsym

# Use full path to the driver library to avoid errors like
# [01000][unixODBC][Driver Manager]Can't open lib 'psqlodbca.so' : file not found
psqlodbca_lib=$(dpkg -L odbc-postgresql | grep -F 'psqlodbca.so')
sudo sed -i'' -e "s@^Driver=psqlodbca\.so@Driver=$psqlodbca_lib@" /etc/odbcinst.ini

echo Contents of /etc/odbcinst.ini
echo '---------------------------------- >8 --------------------------------------'
cat /etc/odbcinst.ini
echo '---------------------------------- >8 --------------------------------------'
