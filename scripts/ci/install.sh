#!/bin/bash
# Run install actions for SOCI build in CI builds
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
source ${SOCI_SOURCE_DIR}/scripts/ci/common.sh

packages_to_install="${SOCI_CI_PACKAGES} libc6-dbg"
if [ "${WITH_BOOST}" != OFF ]; then
    packages_to_install="$packages_to_install  libboost-dev libboost-date-time-dev"
fi

codename=$(lsb_release --codename --short)
# Enable the `-dbgsym` repositories.
echo "deb http://ddebs.ubuntu.com ${codename} main restricted universe multiverse
deb http://ddebs.ubuntu.com ${codename}-updates main restricted universe multiverse" | \
sudo tee --append /etc/apt/sources.list.d/ddebs.list >/dev/null

# Import the debug symbol archive signing key from the Ubuntu server.
# Note that this command works only on Ubuntu 18.04 LTS and newer.
sudo apt-get install -qq -y ubuntu-dbgsym-keyring

sudo apt-get update -qq -y
sudo apt-get install -qq -y ${packages_to_install}

install_script="${SOCI_SOURCE_DIR}/scripts/ci/install_${SOCI_CI_BACKEND}.sh"
if [ -x ${install_script} ]; then
    echo "Running ${install_script}"
    ${install_script}
fi
