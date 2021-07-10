#!/bin/bash -e
# Run before_script actions for SOCI build in CI builds
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
source ${SOCI_SOURCE_DIR}/scripts/ci/common.sh

before_script="${SOCI_SOURCE_DIR}/scripts/ci/before_build_${SOCI_CI_BACKEND}.sh"
if [ -x ${before_script} ]; then
    echo "Running ${before_script}"
    ${before_script}
fi
