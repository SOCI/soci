#!/bin/bash -e
# Install ODBC libraries for SOCI in CI builds
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
source ${SOCI_SOURCE_DIR}/scripts/ci/common.sh

sudo apt-get install -qq \
    tar bzip2 \
    unixodbc-dev \
    odbc-postgresql/xenial odbc-postgresql-dbg/xenial
