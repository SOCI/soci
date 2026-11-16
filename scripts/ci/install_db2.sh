#!/usr/bin/env bash
# Starts DB2 server and installs DB2 CLI driver for SOCI in CI builds
#
# Copyright (c) 2026 Vadim Zeitlin <vz-soci@zeitlins.org>
#
source ${SOCI_SOURCE_DIR}/scripts/ci/common.sh

# Start the server first, as it takes several minutes to create the instance
# and the database: we don't wait for it to become available here but only in
# test_db2.sh, so that this happens in parallel with building SOCI.
#
# Note that the container must be privileged as it changes the kernel IPC
# parameters on startup.
docker run --name ${DB2_CONTAINER} --detach --privileged --publish 50000:50000 \
    --env LICENSE=accept \
    --env DB2INSTANCE=${DB2_USER} --env DB2INST1_PASSWORD="${DB2_PASSWORD}" \
    --env DBNAME=${DB2_DB} \
    ${SOCI_DB2_IMAGE}

# Install the client side: IBM Data Server Driver for ODBC and CLI contains
# both the headers and the library needed for building the backend.
clidriver_url=https://public.dhe.ibm.com/ibmdl/export/pub/software/data/db2/drivers/odbc_cli/linuxx64_odbc_cli.tar.gz
echo "Installing DB2 CLI driver from ${clidriver_url}"
sudo mkdir -p $(dirname ${DB2_CLIDRIVER_DIR})
wget $SOCI_WGET_OPTIONS -O - ${clidriver_url} | \
    sudo tar -C $(dirname ${DB2_CLIDRIVER_DIR}) -xzf -

if [ ! -f ${DB2_CLIDRIVER_DIR}/include/sqlcli1.h ]; then
    echo "DB2 CLI driver installation failed, ${DB2_CLIDRIVER_DIR} contents:"
    ls -lR ${DB2_CLIDRIVER_DIR}
    exit 1
fi
