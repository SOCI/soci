# Vagrant SOCI

[Vagrant](https://www.vagrantup.com/) used to build and provision
virtual environments for SOCI development.

## Features

* Ubuntu 14.04 (Trusty) virtual machine
* Multi-machine set-up with three VMs: `soci`, `oracle`, `db2`.
* `soci.vm`:
  * build essentials
  * core dependencies
  * backend dependencies
  * FOSS databases installed with sample `soci` user and instance pre-configured
  * during provision, automatically clones and builds SOCI from `master` branch.
* `oracle.vm`:
  * *TODO*: provision with Oracle XE
* `db2.vm`:
  * IBM DB2 Express-C 9.7 installed from http://archive.canonical.com packages.
  * *TODO*: Vagrant networking settings.

## Prerequisites

* Speedy broadband, time and coffee.
* Recommended 4GB or much more RAM (tested with 16GB only).

### SOCI DB2 backend

The `soci.vm` will be configured properly to build the DB2 backend only if
it is provisioned with complete DB2 CLI client (libraries and headers).
You need to download "IBM Data Server Driver Package (DS Driver)" manually
and make it visible to Vagrant:

  1. Go to http://www-01.ibm.com/support/docview.wss?uid=swg21385217
  2. Download "IBM Data Server Driver Package (DS Driver)"
  3. Copy the package to `{SOCI SOURCE TREE ROOT}/tmp` directory.

### SOCI DB2 backend

*TODO*

## Usage

* [Boot](https://docs.vagrantup.com/v2/getting-started/up.html)
```
vagrant up # boots all three VMs: soci, db2, oracle (to be provided)
vagrant up soci # boots only soci.vm
vagrant up db2 # boots only db2.vm
```
First time you run it, be patient as Vagrant downloads VM box and
provisions it installing all the necessary packages.

* You can SSH into the machine
```
vagrant ssh soci
```

* Develop
```
cd $SOCI_HOME
git pull origin master
# edit - build - test - commit
```

* [Teardown](https://docs.vagrantup.com/v2/getting-started/teardown.html)
```
vagrant {suspend|halt|destroy} soci
```

Check Vagrant [command-line interface](https://docs.vagrantup.com/v2/cli/index.html)
for complete list of commands.

### Environment variables

The variables available to the `vagrant` user on the virtual machine(s):

* `SOCI_HOME` is where SOCI master is cloned
* `SOCI_USER` default database user and database name
* `SOCI_PASS` default database password for both, `SOCI_USER` and root/sysdba
  of particular database.

Note, those variables are also used by provision scripts to set up databases.

## Troubleshooting

* Analyze `vagrant up` output.
* On Windows, prefer `vagrant ssh` from inside MinGW Shell where  `ssh.exe` is available or
  learn how to use Vagrant with PuTTY.
* If you modify any of `bin/vagrant/*.sh` scripts, **ensure** they have unified
  end-of-line characters to `LF` only. Othwerise, provisioning steps may fail.
