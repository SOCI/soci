# -*- mode: ruby -*-
# vi: set ft=ruby :

# Vagrant virtual development environments for SOCI

# All Vagrant configuration is done below.
# The "2" in Vagrant.configure configures the configuration version.
# Please don't change it unless you know what you're doing.
Vagrant.configure(2) do |config|
  # The most common configuration options are documented and commented below.
  # For a complete reference, please see the online documentation at
  # https://docs.vagrantup.com.

  # Every Vagrant development environment requires a box. You can search for
  # boxes at https://atlas.hashicorp.com/search.
  config.vm.box = "ubuntu/trusty64"

  # Enable automatic box update checking.
  config.vm.box_check_update = true

  # Configure database box with IBM DB2 Express-C
  config.vm.define "db2" do |db2|
    db2.vm.hostname = "vmdb2"
    db2.vm.network "private_network", type: "dhcp"
    # Access to DB2 instance from host
    db2.vm.network :forwarded_port, host: 50000, guest: 50000
    db2.vm.provider :virtualbox do |vb|
      vb.customize ["modifyvm", :id, "--memory", "1024"]
    end
    scripts = [
      "bootstrap.sh",
      "db2.sh"
    ]
    scripts.each { |script|
      db2.vm.provision :shell, privileged: false, :path => "bin/vagrant/" << script
    }
  end

  # TODO
  # Configure database box with Oracle XE
  # config.vm.define "oracle" do |oracle|
  #   oracle.vm.provision "database", type: "shell" do |s|
  #     s.inline = "echo Installing Oracle'"
  #   end
  # end
  #

  # Configure main SOCI development box with build essentials, OSS DBs
  config.vm.define "soci" do |soci|
    soci.vm.hostname = "vmsoci"
    soci.vm.network "private_network", type: "dhcp"
    soci.vm.provider :virtualbox do |vb|
      vb.customize ["modifyvm", :id, "--memory", "1024"]
    end
    scripts = [
      "bootstrap.sh",
      "devel.sh",
      "db2cli.sh",
      "firebird.sh",
      "mysql.sh",
      "postgresql.sh",
      "build.sh"
    ]
    scripts.each { |script|
      soci.vm.provision :shell, privileged: false, :path => "bin/vagrant/" << script
    }
  end
end
