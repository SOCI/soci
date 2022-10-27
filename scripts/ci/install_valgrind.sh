#!/usr/bin/env bash
# Install Valgrind for SOCI in CI builds
#
# Copyright (c) 2020 Vadim Zeitlin <vz-soci@zeitlins.org>
#
source ${SOCI_SOURCE_DIR}/scripts/ci/common.sh

run_apt install valgrind
