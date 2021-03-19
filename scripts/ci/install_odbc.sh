#!/bin/bash -e
# Install ODBC libraries for SOCI in CI builds
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
source ${SOCI_SOURCE_DIR}/scripts/ci/common.sh

sudo apt-get install -qq \
    tar bzip2 \
    unixodbc unixodbc-dev \
    odbc-postgresql/xenial odbc-postgresql-dbg/xenial

# Use full path to the driver library to avoid errors like
# [01000][unixODBC][Driver Manager]Can't open lib 'psqlodbca.so' : file not found
psqlodbca_lib=$(dpkg -L odbc-postgresql | grep -F 'psqlodbca.so')
sudo sed -i'' -e "s@^Driver=psqlodbca\.so@Driver=$psqlodbca_lib@" /etc/odbcinst.ini

echo Contents of /etc/odbcinst.ini
echo '---------------------------------- >8 --------------------------------------'
cat /etc/odbcinst.ini
echo '---------------------------------- >8 --------------------------------------'
