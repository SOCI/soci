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

  # Configure main SOCI development box with build essentials, OSS DBs
  config.vm.define "soci" do |soci|
    scripts = [
      "bootstrap.sh",
      "devel.sh",
      "firebird.sh",
      "mysql.sh",
      "postgresql.sh",
      "build.sh"
    ]
    scripts.each { |script|
      soci.vm.provision :shell, privileged: false, :path => "bin/vagrant/" << script
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
  # # Configure database box with IBM DB2 Express-C
  # config.vm.define "db2" do |db2|
  #   db2.vm.provision "database", type: "shell" do |s|
  #     s.inline = "echo Installing DB2'"
  #   end
  # end
end
