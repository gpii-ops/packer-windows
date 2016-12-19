# Packer templates for Windows

This repository contains Packer templates that can be used to create Windows Vagrant boxes. These boxes will be based on Evaluation copies of Windows which will expire after 90 days. Please be sure to consult and comply with [Microsoft's licensing agreements](https://www.microsoft.com/en-us/evalcenter/evaluate-windows-10-enterprise).

## Current Boxes

* [Windows 10 Evaluation](https://atlas.hashicorp.com/inclusivedesign/boxes/windows10-eval)

## Building Boxes

To build any of the boxes you will need to have the following installed:

* [Packer](https://www.packer.io/)
* [VirtualBox](https://www.virtualbox.org/)
* [Vagrant](https://www.vagrantup.com/)

Once the requirements have been met a build can be started by issuing these commands:

```
git submodule init
git submodule update
packer build -only=virtualbox-iso windows_10.json
```

The build process can take several hours depending on the type of hardware and network connection being used. You can expect the following:
* A Windows Enterprise ISO image will be downloaded from Microsoft's servers
* An unattended Windows installation will be carried out
* Security updates will be downloaded and applied
* Additional third party packages will be installed

The resulting Vagrant box will be at least 6.5GB in size. 

It also necessary to have plenty of space in the /tmp directory for temporary files (>10GB). You can ask packer to use a different temporary directory with more space through the TMPDIR variable:

```
TMPDIR=/other_tmp packer build -only=virtualbox-iso windows_10.json
```

## Start a VM 

To start a VM make sure the box as been added:

```
vagrant box add --name "windows10-eval" windows_10_virtualbox.box
vagrant init windows10-eval
```

And then a new instance can be created:

```
vagrant up
```

## Run commands

To run commands remotely from your host operating system you will need to first install the [vagrant-winrm](https://github.com/criteo/vagrant-winrm) plugin:

```
vagrant plugin install vagrant-winrm
```

Then you can run CLI processes using the following command and obtain their output and exit codes in your host's terminal:

```
vagrant winrm -c "do.ps1 -c '<your command>'"
```

* Commands need to be in quotes and passed to the ``do.ps1`` script using its ``-c`` option, for example: 
  * ``vagrant winrm -c "do.ps1 -c 'choco install rclone -y'"``
  * ``vagrant winrm -c "do.ps1 -c 'node tests\UnitTests.js'"``
* All commands will run in the ``C:\vagrant`` directory which is the shared folder between your host OS and the Windows VM
* Commands need to return the shell prompt and not capture it indefinitely
* This is most useful for processes that will spawn applications which require desktop access


## Internals: DoIt

To run commands over WinRM and get access to the console session (`query session`) this box employs DoIt (http://www.chiark.greenend.org.uk/~sgtatham/doit), which is a remote-execution daemon created by Simon Tatham. Without it, commands sent through WinRM are executed in a separate session and do not interact with the desktop (no visible windows).

The DoIt client (doitclient.exe) is invoked over WinRM, connects locally to the DoIt server (doit.exe) which in turns executes the commands in the console session, retrieves the standard output and correctly relays the exit code back to WinRM.
