#!/usr/bin/env bash
# Part of Vagrant virtual development environments for SOCI

# Builds and tests SOCI from git master branch
source /vagrant/bin/vagrant/common.env

# Clone SOCI locally on Linux filesystem, instead of VM shared folder.
# Otherwise, CMake will fail:
# CMake Error: cmake_symlink_library: System Error: Protocol error
# Explanation from https://github.com/mitchellh/vagrant/issues/713
# The VirtualBox shared folder filesystem doesn't allow symlinks, unfortunately.
# Your only option is to deploy outside of the shared folders.
if [[ ! -d "${SOCI_HOME}" && ! -d "${SOCI_HOME}/.git" ]] ; then
  echo "Build: cloning SOCI master"
  git clone https://github.com/SOCI/soci.git ${SOCI_HOME}
fi

if [[ ! -d "${SOCI_HOME}/_build" ]] ; then
  mkdir ${SOCI_HOME}/_build
fi

echo "Build: building SOCI"
cd ${SOCI_HOME}/_build && \
cmake \
    -DSOCI_TESTS=ON \
    -DSOCI_STATIC=OFF \
    -DSOCI_DB2=ON \
    -DSOCI_ODBC=OFF \
    -DSOCI_ORACLE=OFF \
    -DSOCI_EMPTY=ON \
    -DSOCI_FIREBIRD=OFF \
    -DSOCI_MYSQL=ON \
    -DSOCI_POSTGRESQL=ON \
    -DSOCI_SQLITE3=ON \
    -DSOCI_FIREBIRD_TEST_CONNSTR:STRING="service=LOCALHOST:/tmp/soci.fdb user=${SOCI_USER} password==${SOCI_PASS}" \
    -DSOCI_MYSQL_TEST_CONNSTR:STRING="host=localhost db=${SOCI_USER} user=${SOCI_USER} password==${SOCI_PASS}" \
    -DSOCI_POSTGRESQL_TEST_CONNSTR:STRING="host=localhost port=5432 dbname=${SOCI_USER} user=${SOCI_USER} password==${SOCI_PASS}" \
    .. && \
make
# Do not run tests during provisioning, thay may fail terribly, so just build
# and let to run them manually after developer vagrant ssh'ed to the VM.
#ctest -V --output-on-failure .
echo "Build: building DONE"
