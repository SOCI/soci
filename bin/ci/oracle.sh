# Definitions used by SOCI when building Oracle backend at travis-ci.org
#
# Copyright (c) 2015 Vadim Zeitlin <vz-soci@zeitlins.org>
#
# Notice that this file is not executable, it is supposed to be sourced from
# the other files.

ORACLE_HOME=/opt/instantclient_11_2
export ORACLE_HOME

LD_LIBRARY_PATH=${ORACLE_HOME}:${LD_LIBRARY_PATH}
export LD_LIBRARY_PATH
