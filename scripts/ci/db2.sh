# Definitions used by SOCI when testing DB2 in CI builds
#
# Copyright (c) 2026 Vadim Zeitlin <vz-soci@zeitlins.org>
#
# Notice that this file is not executable, it is supposed to be sourced from
# the other files.

# Docker image used for the server, override it to test with another version.
: ${SOCI_DB2_IMAGE:=icr.io/db2_community/db2:latest}

# This is arbitrary.
export DB2_CONTAINER=soci-db2

# The instance user is created by the container using the given password.
export DB2_USER=db2inst1
export DB2_PASSWORD=db2inst1

# Database created by the container on startup and used by the tests (notice
# that DB2 database names can't be longer than 8 characters).
export DB2_DB=SOCITEST

# Directory where IBM Data Server Driver for ODBC and CLI is installed: this is
# one of the locations searched by FindDB2.cmake.
export DB2_CLIDRIVER_DIR=/opt/ibm/clidriver

# Return the CLI connection string for the test database.
db2_connstr()
{
    echo "DATABASE=${DB2_DB};HOSTNAME=127.0.0.1;PORT=50000;PROTOCOL=TCPIP;UID=${DB2_USER};PWD=${DB2_PASSWORD}"
}

# Execute the DB2 CLP command given as argument as the instance user inside the
# container.
db2_exec()
{
    docker exec ${DB2_CONTAINER} su - ${DB2_USER} -c "$*"
}
