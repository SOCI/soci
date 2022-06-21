#!/usr/bin/env bash
# Run SOCI tests in CI builds
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
source ${SOCI_SOURCE_DIR}/scripts/ci/common.sh

# run tests from the build directory
cd ${builddir}

# run the tests
SCRIPT=${SOCI_SOURCE_DIR}/scripts/ci/test_${SOCI_CI_BACKEND}.sh
if [ -x ${SCRIPT} ]; then
  # use the custom script for this backend
  echo "Running ${SCRIPT}"
  ${SCRIPT}
else
  run_test
fi
