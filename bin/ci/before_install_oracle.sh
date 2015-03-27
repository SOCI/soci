#!/bin/bash -e
# Install Oracle client libraries for SOCI at travis-ci.org
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
source ${TRAVIS_BUILD_DIR}/bin/ci/common.sh
source ${TRAVIS_BUILD_DIR}/bin/ci/oracle.sh

sudo apt-get install -qq tar bzip2 libaio1

wget https://www.dropbox.com/s/30ex7s0d1pru0ur/instantclient_11_2-linux-x64.tar.bz2
tar -jxvf instantclient_11_2-linux-x64.tar.bz2
sudo mkdir -p /opt
sudo mv instantclient_11_2 /opt
