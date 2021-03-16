#!/bin/bash -e
# Install Valgrind for SOCI at travis-ci.org
#
# Copyright (c) 2020 Vadim Zeitlin <vz-soci@zeitlins.org>
#
source ${SOCI_SOURCE_DIR}/scripts/ci/common.sh

sudo apt-get install -qq valgrind