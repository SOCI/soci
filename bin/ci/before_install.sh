#!/bin/bash
# Run before_intsall actions for SOCI build at travis-ci.org
# Mateusz Loskot <mateusz@loskot.net>, http://github.com/SOCI
#
source ./bin/ci/common.sh
# Install dependencies
echo "$(tmstamp) *** before_install::apt-get starting $(date) ***"
#wget http://oss.oracle.com/el4/RPM-GPG-KEY-oracle -O - | sudo apt-key add -
#sudo bash -c 'echo "deb https://oss.oracle.com/debian unstable main non-free" >> /etc/apt/sources.list'
sudo apt-key adv --recv-keys --keyserver keyserver.ubuntu.com 16126D3A3E5C1192
sudo apt-get update -qq
#if [ `uname -m` = x86_64 ]; then sudo apt-get install -qq --force-yes ia32-libs ia32-libs-multiarch fi
sudo apt-get install -qq \
    libstdc++5 libboost-dev libboost-date-time-dev \
    libmyodbc unixodbc-dev odbc-postgresql \
    firebird2.5-super firebird2.5-dev
echo "$(tmstamp) *** before_install::apt-get finished $(date) ***"

echo "$(tmstamp) *** before_install::config starting $(date) ***"
# Configure Firebird
# See: Non-interactive setup for travis-ci.org 
# http://tech.groups.yahoo.com/group/firebird-support/message/120883
#sudo dpkg-reconfigure -f noninteractive firebird2.5-super
sudo sed /ENABLE_FIREBIRD_SERVER=/s/no/yes/ -i /etc/default/firebird2.5
cat /etc/default/firebird2.5 | grep ENABLE_FIREBIRD_SERVER
sudo service firebird2.5-super start
# Configure ODBC
sudo odbcinst -i -d -f /usr/share/libmyodbc/odbcinst.ini
echo "$(tmstamp) *** before_install::config starting $(date) ***"
