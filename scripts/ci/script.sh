#!/bin/bash
# Run test script actions for SOCI build at travis-ci.org
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
source ${SOCI_SOURCE_DIR}/scripts/ci/common.sh

# prepare build directory
builddir="${SOCI_SOURCE_DIR}/_build"
mkdir -p ${builddir}
cd ${builddir}

# build and run tests
SCRIPT=${SOCI_SOURCE_DIR}/scripts/ci/script_${SOCI_TRAVIS_BACKEND}.sh
echo "Running ${SCRIPT}"
${SCRIPT}
