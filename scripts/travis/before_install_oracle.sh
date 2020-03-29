#!/bin/bash
# Script performs non-interactive installation of Oracle XE on Linux
#
# Uses Oracle downloader and installer from https://github.com/cbandy/travis-oracle
#
# set -ex
source ${TRAVIS_BUILD_DIR}/scripts/travis/oracle.sh

docker run --name $ORACLE_CONT -e ORACLE_PWD="$ORACLE_PWD" vitorfec/oracle-xe-18c

# We need to wait until the database creation process finishes, so steal this
# trick from https://github.com/elhigu/travis-node-oracle-18-xe to do it.
echo -n 'Waiting for Oracle database creation: '
until
    docker exec $ORACLE_CONT sh -c "echo 'SELECT 13376411 FROM DUAL;' |
    sqlplus -S -L sys/$ORACLE_PWD AS SYSDBA 2>&1 |
    grep --quiet 13376411"; do
    echo -n .
    sleep 10
done
echo 'done!'

# We also need to copy the headers and libraries from the container to the
# machine where we're building.
mkdir -p $ORACLE_HOME/lib
for f in libclntsh.so libclntsh.so.18.1 \
    libclntshcore.so libclntshcore.so.18.1 \
    libocci.so libocci.so.18.1 \
    libipc1.so libmql1.so libnnz18.so libons.so; do
    docker cp $ORACLE_CONT:$ORACLE_HOME/lib/$f $ORACLE_HOME/lib
done

docker cp $ORACLE_CONT:$ORACLE_HOME/sdk/include $ORACLE_HOME
