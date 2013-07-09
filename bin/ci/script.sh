#!/bin/bash
# Run test script actions for SOCI build at travis-ci.org
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
source ./bin/ci/common.sh

# prepare build directory
builddir="${TRAVIS_BUILD_DIR}/src/_build"
mkdir -p ${builddir} && cd ${builddir} \
    || { echo "'${builddir}' does not exist"; exit 1; }

# build and run tests
script="./bin/ci/script_${SOCI_TRAVIS_BACKEND}.sh"
[ -x ${script} ] && ${script} \
    || { echo "'${script}' does not exist"; exit 1; }
