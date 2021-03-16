#!/bin/bash
# Run before_install actions for SOCI build in CI builds
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
source ${SOCI_SOURCE_DIR}/scripts/ci/common.sh

packages_to_install=libc6-dbg
if [ "${WITH_BOOST}" != OFF ]; then
    packages_to_install="$packages_to_install  libboost-dev libboost-date-time-dev"
fi

sudo apt-get update -qq -y
sudo apt-get install -qq -y ${packages_to_install}

before_install="${SOCI_SOURCE_DIR}/scripts/ci/before_install_${SOCI_CI_BACKEND}.sh"
if [ -x ${before_install} ]; then
    echo "Running ${before_install}"
    ${before_install}
fi
