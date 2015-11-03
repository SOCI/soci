#!/usr/bin/env bash
# Part of Vagrant virtual development environments for SOCI

# Pre-installation
echo "Bootstrap: setting common environment in /etc/profile.d/vagrant-soci.sh"
sudo sh -c "cat /vagrant/bin/vagrant/common.env > /etc/profile.d/vagrant-soci.sh"
export DEBIAN_FRONTEND="noninteractive"
# Installation
# TODO: Switch to apt-fast when it is avaiable for Trusty
sudo apt-get update -y -q
