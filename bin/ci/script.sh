#!/bin/bash -e
# Run test script actions for SOCI build at travis-ci.org
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
source ./bin/ci/common.sh

# prepare build directory
builddir="${TRAVIS_BUILD_DIR}/_build"
mkdir -p ${builddir} && cd ${builddir} \
    || { echo "'${builddir}' does not exist"; exit 1; }

echo $PWD
# build and run tests
script="${TRAVIS_BUILD_DIR}/bin/ci/script_${SOCI_TRAVIS_BACKEND}.sh"
[ -x ${script} ] || { echo "'${script}' does not exist"; exit 1; }
${script}
