#!/bin/bash
# Run before_install actions for SOCI build at travis-ci.org
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
source ${TRAVIS_BUILD_DIR}/scripts/travis/common.sh

wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | gpg --dearmor - | sudo tee /etc/apt/trusted.gpg.d/kitware.gpg >/dev/null
sudo apt-add-repository -y 'deb https://apt.kitware.com/ubuntu/ xenial main'
sudo apt-get update -qq -y
sudo apt-get install -y libboost-dev libboost-date-time-dev valgrind cmake
# export PATH=/usr/local/bin:$PATH
sudo rm -r /usr/local/cmake-12.4/
echo "Test"
ls /usr/local/cmake-12.4/bin
apt-cache madison cmake
cmake --version

before_install="${TRAVIS_BUILD_DIR}/scripts/travis/before_install_${SOCI_TRAVIS_BACKEND}.sh"
if [ -x ${before_install} ]; then
	echo "Running ${before_install}"
    ${before_install}
fi
