# Definitions used by SOCI when building Oracle backend in CI builds
#
# Copyright (c) 2015 Vadim Zeitlin <vz-soci@zeitlins.org>
#
# Notice that this file is not executable, it is supposed to be sourced from
# the other files.

# This is arbitrary.
export ORACLE_CONTAINER=oracle-11g

# We use the same name for the path inside and outside the container.
export ORACLE_HOME=/u01/app/oracle/product/11.2.0/xe
export ORACLE_SID=XE

# Add path to Oracle libraries.
export LD_LIBRARY_PATH=$ORACLE_HOME/lib

# Execute any command in the Oracle container: pass the command with its
# arguments directly to the function.
oracle_exec()
{
    docker exec ${ORACLE_CONTAINER} "$@"
}

# Execute SQLPlus in the Oracle container: pass the extra arguments to the
# command to this function and its input on stdin.
oracle_sqlplus()
{
    docker exec --interactive --env ORACLE_HOME=${ORACLE_HOME} --env ORACLE_SID=${ORACLE_SID} ${ORACLE_CONTAINER} "$ORACLE_HOME/bin/sqlplus" -S -L "$@"
}
