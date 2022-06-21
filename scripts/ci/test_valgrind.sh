#!/usr/bin/env bash
# Builds SOCI for use with Valgrind in CI builds
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
# Copyright (c) 2015 Sergei Nikulov <sergey.nikulov@gmail.com>
# Copyright (c) 2021 Vadim Zeitlin <vz-soci@zeitlins.org>
#
source ${SOCI_SOURCE_DIR}/scripts/ci/common.sh

valgrind --leak-check=full --suppressions=${SOCI_SOURCE_DIR}/valgrind.suppress --error-exitcode=1 --trace-children=yes ctest -V --output-on-failure "$@" .
