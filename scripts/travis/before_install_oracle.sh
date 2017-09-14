#!/bin/bash
# Script performs non-interactive installation of Oracle XE on Linux
#
# Uses Oracle downloader and installer from https://github.com/cbandy/travis-oracle
#
# set -ex
if [[ "$TRAVIS_PULL_REQUEST" != "false" ]] ; then
    echo "Skipping Oracle installation for PR builds"
    exit 0
fi

source ${TRAVIS_BUILD_DIR}/scripts/travis/oracle.sh

# Install Oracle and travis-oracle requirements
sudo apt-get install -y libaio1 rpm

curl -s -o $HOME/.nvm/nvm.sh https://raw.githubusercontent.com/creationix/nvm/v0.31.0/nvm.sh
source $HOME/.nvm/nvm.sh
nvm install stable
node --version

# Install Oracle
wget 'https://github.com/cbandy/travis-oracle/archive/v2.0.2.tar.gz'
mkdir -p .travis/oracle
tar x -C .travis/oracle --strip-components=1 -f v2.0.2.tar.gz

bash .travis/oracle/download.sh
bash .travis/oracle/install.sh
