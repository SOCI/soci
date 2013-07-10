#!/bin/bash
# Run test script for SOCI backend empty at travis-ci.org
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
source ${TRAVIS_BUILD_DIR}/bin/ci/common.sh
echo $TRAVIS_BUILD_DIR
echo $PWD
cmake ..

