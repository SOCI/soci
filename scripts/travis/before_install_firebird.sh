#!/bin/bash -e
# Install Firebird server for SOCI at travis-ci.org
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
source ${TRAVIS_BUILD_DIR}/scripts/travis/common.sh

sudo apt-get install -qq expect firebird2.5-super firebird2.5-dev

export DEBIAN_FRONTEND="readline"
# Expect script feeding dpkg-reconfigure prompts
sudo /usr/bin/expect - << ENDMARK > /dev/null
spawn dpkg-reconfigure firebird2.5-super -freadline
expect "Enable Firebird server?"
send "Y\r"

expect "Password for SYSDBA:"
send "masterkey\r"

# done
expect eof
ENDMARK
# End of Expect script
export DEBIAN_FRONTEND="noninteractive"
echo "Firebird: cat /etc/firebird/2.5/SYSDBA.password"
sudo cat /etc/firebird/2.5/SYSDBA.password | grep ISC_
echo
echo "Firebird: restarting"
sudo service firebird2.5-super restart
echo "Firebird: DONE"
