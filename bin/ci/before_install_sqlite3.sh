#!/bin/bash
# Run before_install for SOCI backend SQLite3 at travis-ci.org
#
# Copyright (c) 2013 Mateusz Loskot <mateusz@loskot.net>
#
source ./bin/ci/common.sh
sudo apt-get install -qq libsqlite3-dev

