#!/usr/bin/env bash
# Tests SOCI DB2 backend in CI builds
#
# Copyright (c) 2013 Brian R. Toonen <toonen@alcf.anl.gov>
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
# Copyright (c) 2021 Vadim Zeitlin <vz-soci@zeitlins.org>
#
source ${SOCI_SOURCE_DIR}/scripts/ci/common.sh

# The server was started by install_db2.sh, wait until it finishes setting up.
echo 'Waiting for DB2 setup to complete...'
num_tries=1
wait_time=10 # seconds
while true; do
    if docker logs ${DB2_CONTAINER} 2>&1 | grep -q 'Setup has completed'; then
        echo 'DB2 is available now'
        break
    fi

    if ! docker inspect --format '{{.State.Running}}' ${DB2_CONTAINER} | grep -q true; then
        echo 'DB2 container is not running any more, its log follows:'
        docker logs ${DB2_CONTAINER}
        exit 1
    fi

    if [[ $num_tries -gt 90 ]]; then
        echo 'Timed out waiting for DB2 setup, its log follows:'
        docker logs ${DB2_CONTAINER}
        exit 1
    fi

    sleep $wait_time
    ((num_tries++))
done

# By default DB2 waits for locks forever, which would make the job hang if a
# test leaves a lock behind, so set a finite timeout. This requires
# reactivating the database to take effect.
db2_exec "db2 update db cfg for ${DB2_DB} using LOCKTIMEOUT 60"
db2_exec "db2 deactivate db ${DB2_DB}" || true
db2_exec "db2 activate db ${DB2_DB}"

echo 'Testing connection to the database:'
db2_exec "db2 connect to ${DB2_DB} && db2 'select service_level from sysibmadm.env_inst_info' && db2 connect reset"

LSAN_OPTIONS=suppressions=${SOCI_SOURCE_DIR}/scripts/suppressions/lsan-db2 run_test
