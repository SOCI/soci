#!/usr/bin/env bash
# Builds SOCI for use with Valgrind in CI builds
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
# Copyright (c) 2015 Sergei Nikulov <sergey.nikulov@gmail.com>
#
source ${SOCI_SOURCE_DIR}/scripts/ci/common.sh

# Note that we don't use the default options here, as we don't want to turn
# off all the backends (nor to enable ASAN which is incompatible with Valgrind).
cmake ${SOCI_COMMON_CMAKE_OPTIONS} \
    -DSOCI_ODBC=OFF \
    ..

run_make
