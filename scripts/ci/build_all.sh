#!/usr/bin/env bash
# Builds SOCI with all backends.
#
# Copyright (c) 2021 Vadim Zeitlin <vz-soci@zeitlins.org>
#
source ${SOCI_SOURCE_DIR}/scripts/ci/common.sh

# We don't use the default options here, as we don't want to turn off all the
# backends.
cmake ${SOCI_COMMON_CMAKE_OPTIONS} \
    ..

run_make
