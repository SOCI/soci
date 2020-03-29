# Definitions used by SOCI when building Oracle backend at travis-ci.org
#
# Copyright (c) 2015,2020 Vadim Zeitlin <vz-soci@zeitlins.org>
#
# Notice that this file is not executable, it is supposed to be sourced from
# the other files.

# Oracle installation directory used by official Docker container that we also
# use on the machine running the container for simplicity.
export ORACLE_HOME=/opt/oracle/product/18c/dbhomeXE

# Name of Oracle container to create.
export ORACLE_CONT=oracle-xe-18c

# Oracle DBA password specified during the installation.
export ORACLE_PWD=soci_tiger
