#!/usr/bin/env bash
# Tests SOCI FB backend in CI builds
#
# Copyright (c) 2021 Ilya Sinitsyn <isinitsyn@tt-solutions.com>
#
source ${SOCI_SOURCE_DIR}/scripts/ci/common.sh

LSAN_OPTIONS=suppressions=${SOCI_SOURCE_DIR}/scripts/suppress_firebird.txt run_test
