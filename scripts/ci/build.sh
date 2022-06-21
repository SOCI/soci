#!/usr/bin/env bash
# Build SOCI in CI builds
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
source ${SOCI_SOURCE_DIR}/scripts/ci/common.sh

# prepare build directory
mkdir -p ${builddir}
cd ${builddir}

# do build
SCRIPT=${SOCI_SOURCE_DIR}/scripts/ci/build_${SOCI_CI_BACKEND}.sh
echo "Running ${SCRIPT}"
${SCRIPT}
