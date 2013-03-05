#!/bin/bash
# Common definitions used by SOCI build scripts at travis-ci.org
# Mateusz Loskot <mateusz@loskot.net>, http://github.com/SOCI
#
set -e

if [[ "$TRAVIS" != "true" ]] ; then
	echo "Running this script makes no sense outside of travis-ci.org"
	exit 1
fi
# Functions
tmstamp() { echo -n "[$(date '+%H:%M:%S')]" ; }
# Environment
NUMTHREADS=4
if [[ -f /sys/devices/system/cpu/online ]]; then
	# Calculates 1.5 times physical threads
	NUMTHREADS=$(( ( $(cut -f 2 -d '-' /sys/devices/system/cpu/online) + 1 ) * 15 / 10  ))
fi
export NUMTHREADS
export ORACLE_HOME=/usr/lib/oracle/xe/app/oracle/product/10.2.0/client
