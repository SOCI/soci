#!/usr/bin/env bash
# Part of Vagrant virtual development environments for SOCI

# Pre-installation
echo "Bootstrap: updating environment in /home/vagrant/.bashrc"
echo "source /vagrant/bin/vagrant/common.env" >> /home/vagrant/.bashrc
export DEBIAN_FRONTEND="noninteractive"
# Installation
# TODO: Switch to apt-fast when it is avaiable for Trusty
sudo apt-get update -y -q
