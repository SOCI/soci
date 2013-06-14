#!/bin/bash
# Run before_intsall actions for SOCI build at travis-ci.org
# Mateusz Loskot <mateusz@loskot.net>, http://github.com/SOCI
#
source ./bin/ci/common.sh
# Install dependencies
echo "$(tmstamp) *** before_install::apt-get starting $(date) ***"
#sudo apt-get install -qq apt-transport-https
#wget http://oss.oracle.com/el4/RPM-GPG-KEY-oracle -O - | sudo apt-key add -
#sudo bash -c 'echo "deb https://oss.oracle.com/debian unstable main non-free" >> /etc/apt/sources.list'
sudo bash -c 'echo "deb http://archive.canonical.com/ubuntu precise partner" >> /etc/apt/sources.list'
sudo apt-key adv --recv-keys --keyserver keyserver.ubuntu.com 16126D3A3E5C1192
sudo apt-get update -qq
#if [ `uname -m` = x86_64 ]; then sudo apt-get install -qq --force-yes ia32-libs ia32-libs-multiarch fi
sudo apt-get install -qq \
    tar bzip2 \
    libstdc++5 \
    libaio1 \
    libboost-dev libboost-date-time-dev \
    libmyodbc odbc-postgresql \
    firebird2.5-super firebird2.5-dev
echo "$(tmstamp) *** before_install::apt-get finished $(date) ***"

echo "$(tmstamp) *** before_install::oracle starting $(date) ***"

if wget http://brzuchol.loskot.net/software/oracle/instantclient_11_2-linux-x64-mloskot.tar.bz2 ; then
    tar -jxf instantclient_11_2-linux-x64-mloskot.tar.bz2
    sudo mkdir -p /opt
    sudo mv instantclient_11_2 /opt
    sudo ln -s ${ORACLE_HOME}/libclntsh.so.11.1 ${ORACLE_HOME}/libclntsh.so
    sudo ln -s ${ORACLE_HOME}/libocci.so.11.1 ${ORACLE_HOME}/libocci.so
else
    echo "WARNING: failed to download Orcale distribution.  Oracle will not be tested."
fi
echo "$(tmstamp) *** before_install::oracle finished $(date) ***"

echo "$(tmstamp) *** before_install::db2 starting $(date) ***"
echo "Running apt-get install db2exc"
sudo apt-get install -qq db2exc
echo "Running db2profile and db2rmln"
sudo /bin/sh -c '. ~db2inst1/sqllib/db2profile ; $DB2DIR/cfg/db2rmln'
echo "Setting up db2 users"
echo -e "db2inst1\ndb2inst1" | sudo passwd db2inst1
echo -e "db2fenc1\ndb2fenc1" | sudo passwd db2fenc1
echo -e "dasusr1\ndasusr1" | sudo passwd dasusr1
echo "Configuring DB2 ODBC driver"
if test `getconf LONG_BIT` = "64" ; then
    if test -f /home/db2inst1/sqllib/lib64/libdb2o.so ; then
	DB2_ODBC_DRIVER=/home/db2inst1/sqllib/lib64/libdb2o.so
    else
	echo "ERROR: can't find the 64-bit DB2 ODBC library"
	exit 1
    fi
else
    if test -f /home/db2inst1/sqllib/lib32/libdb2.so ; then
	DB2_ODBC_DRIVER=/home/db2inst1/sqllib/lib32/libdb2.so
    elif test -f /home/db2inst1/sqllib/lib/libdb2.so ; then
	DB2_ODBC_DRIVER=/home/db2inst1/sqllib/lib/libdb2.so
    else
	echo "ERROR: can't find the 32-bit DB2 ODBC library"
	exit 1
    fi
fi
echo "DB2 ODBC driver set to $DB2_ODBC_DRIVER"
echo "$(tmstamp) *** before_install::db2 finished $(date) ***"

echo "$(tmstamp) *** before_install::odbc starting $(date) ***"
# to prevent header file conflicts, unixodbc-dev has to be installed after
# db2rmln is run
sudo apt-get install -qq unixodbc-dev 
echo "$(tmstamp) *** before_install::odbc finished $(date) ***"

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
cat <<EOF | sudo odbcinst -i -d -r 
[DB2]
Description = ODBC for DB2
Driver = $DB2_ODBC_DRIVER
FileUsage = 1
Threading = 0
EOF
echo "$(tmstamp) *** before_install::config finished $(date) ***"
