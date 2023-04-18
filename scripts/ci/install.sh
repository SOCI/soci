#!/bin/sh
# Run install actions for SOCI build in CI builds
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
# Note that this is a /bin/sh script because bash is not installed yet under
# FreeBSD.
. ${SOCI_SOURCE_DIR}/scripts/ci/common.sh

case "$(uname)" in
    Linux)
        # Install the bare minimum for the rest to work if we're running on a
        # minimum system.
        if ! command -v lsb_release > /dev/null; then
            # We can't use run_apt here because it uses sudo which may also be
            # not available yet.
            apt-get $SOCI_APT_OPTIONS update
            apt-get $SOCI_APT_OPTIONS install build-essential lsb-release sudo
        fi

        packages_to_install="cmake libc6-dbg"
        if [ "${WITH_BOOST}" != OFF ]; then
            packages_to_install="$packages_to_install  libboost-dev libboost-date-time-dev"
        fi

        # Get rid of the repositories that we don't need: not only this takes
        # extra time to update, but it also often fails with "Mirror sync in
        # progress" errors.
        for apt_file in `grep -lr microsoft /etc/apt/sources.list.d/`; do sudo rm $apt_file; done

        codename=$(lsb_release --codename --short)
        # Enable the `-dbgsym` repositories.
        echo "deb http://ddebs.ubuntu.com ${codename} main restricted universe multiverse
        deb http://ddebs.ubuntu.com ${codename}-updates main restricted universe multiverse" | \
        sudo tee --append /etc/apt/sources.list.d/ddebs.list >/dev/null

        # Import the debug symbol archive signing key from the Ubuntu server.
        # Note that this command works only on Ubuntu 18.04 LTS and newer.
        run_apt install ubuntu-dbgsym-keyring

        run_apt update
        run_apt install ${packages_to_install}
        ;;

    FreeBSD)
        pkg install -q -y bash cmake
        ;;
esac

install_script="${SOCI_SOURCE_DIR}/scripts/ci/install_${SOCI_CI_BACKEND}.sh"
if [ -x ${install_script} ]; then
    echo "Running ${install_script}"
    ${install_script}
fi
