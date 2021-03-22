#!/bin/bash -e
# Install Firebird server for SOCI in CI builds
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
source ${SOCI_SOURCE_DIR}/scripts/ci/common.sh

case $(lsb_release -sc) in
    trusty | xenial)
        firebird_version=2.5
        firebird_server_package=firebird2.5-super
        firebird_server_service=firebird2.5-super
        firebird_expect_enable=$(cat <<EOF
expect "Enable Firebird server?"
send "yes\r"
EOF
)
        ;;

    focal)
        firebird_version=3.0
        firebird_server_package=firebird3.0-server
        firebird_server_service=firebird3.0
        ;;

    *)
        echo "*** Can't install Firebird: unknown Ubuntu version! ***"
        exit 1
esac

sudo apt-get install -qq expect ${firebird_server_package} firebird-dev

# Default frontend is "noninteractive", which prevents dpkg-reconfigure from
# asking anything at all, so change it. Notice that we must do it via
# environment and not using -f option of dpkg-reconfigure because it is
# overridden by the existing environment variable (which is predefined).
#
# OTOH we do need to set priority to low using -p option below as otherwise we
# wouldn't be asked to change the password after the initial installation.
export DEBIAN_FRONTEND=teletype

# Expect script feeding dpkg-reconfigure prompts
sudo --preserve-env /usr/bin/expect - << ENDMARK
spawn dpkg-reconfigure -plow $firebird_server_package
$firebird_expect_enable

expect "Password for SYSDBA:"
send "masterkey\r"

# done
expect eof
ENDMARK
# End of Expect script

echo "Firebird: cat /etc/firebird/$firebird_version/SYSDBA.password"
sudo grep ISC_ /etc/firebird/$firebird_version/SYSDBA.password
echo
echo "Firebird: restarting"
sudo service $firebird_server_service restart
echo "Firebird: DONE"
