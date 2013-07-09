#!/bin/bash
# Run before_script actions for SOCI build at travis-ci.org
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
source ./bin/ci/common.sh

before_script="./bin/ci/before_script_${SOCI_TRAVIS_BACKEND}.sh"
[ -x ${before_script} ] && ${before_script}

