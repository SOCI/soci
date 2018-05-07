#!/bin/sh
set -x

wget -q https://cmake.org/files/v3.11/cmake-3.11.0-Linux-x86_64.sh
sudo sh cmake-3.11.0-Linux-x86_64.sh -- --skip-license --prefix=/usr/local
cmake --version
