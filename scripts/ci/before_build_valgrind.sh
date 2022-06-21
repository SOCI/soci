#!/usr/bin/env bash
# Sets up environment for SOCI backend in CI builds
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
# Copyright (c) 2015 Sergei Nikulov <sergey.nikulov@gmail.com>
# Copyright (c) 2021 Vadim Zeitlin <vz-soci@zeitlins.org>
#

# We use both MySQL and PostgreSQL in this build.
${SOCI_SOURCE_DIR}/scripts/ci/before_build_mysql.sh
${SOCI_SOURCE_DIR}/scripts/ci/before_build_postgresql.sh
