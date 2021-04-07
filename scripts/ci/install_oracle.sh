#!/bin/bash -e
# Sets up Oracle database for SOCI Oracle backend CI builds.
#
source ${SOCI_SOURCE_DIR}/scripts/ci/common.sh

docker run --name ${ORACLE_CONTAINER} --detach --publish 1521:1521 -e ORACLE_ALLOW_REMOTE=true wnameless/oracle-xe-11g-r2

echo 'Waiting for Oracle startup...'
num_tries=1
wait_time=6 # seconds
while true; do
    if oracle_exec /etc/init.d/oracle-xe status | grep -q 'Instance "XE", status READY'; then
        if echo "SELECT STATUS, DATABASE_STATUS FROM V\$INSTANCE WHERE INSTANCE_NAME='XE';" | \
            oracle_sqlplus sys/oracle AS SYSDBA | grep -q 'OPEN'; then
            echo 'Oracle database is available now'
            break
        fi
    fi

    if [[ $num_tries -gt 50 ]]; then
        echo 'Timed out waiting for Oracle startup'
        break
    fi

    echo "Waiting $wait_time more seconds (attempt #$num_tries)"
    sleep $wait_time
    ((num_tries++))
done

echo 'Oracle log:'
docker logs --timestamps ${ORACLE_CONTAINER}
echo 'Oracle global status:'
oracle_exec /etc/init.d/oracle-xe status
echo 'Oracle instance status:'
echo 'SELECT INSTANCE_NAME, STATUS, DATABASE_STATUS FROM V$INSTANCE;' | \
oracle_sqlplus sys/oracle AS SYSDBA

# Copy Oracle directory, notably containing the headers and the libraries
# needed for the compilation, from the container.
sudo mkdir -p ${ORACLE_HOME}
sudo docker cp ${ORACLE_CONTAINER}:${ORACLE_HOME} ${ORACLE_HOME}/..
