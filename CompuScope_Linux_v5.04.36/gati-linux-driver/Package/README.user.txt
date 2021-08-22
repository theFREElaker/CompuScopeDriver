
================
1. Installation
================

-------
1.1 RPM
-------

Installing an RPM file is usually done in one of those 3 ways :
	- Using the "rpm" command
	- Using the "yum" command
	- Using the graphical package manager (usually opened when double-clikcing the file)
	
When using the "rpm" command, installation requires using the install (-i or --install) or the upgrade (-U or --upgrade) option.
When using the "yum" command, the "install" option is required.

Installing with a command line requires to specify the package file.

-------
1.2 DEB
-------

Installing a DEB file is usually done in one of those 3 ways :
	- Using the "dpkg" command
	- Using the "apt-get" or "apt" command
	- Using the graphical package manager (usually opened when double-clikcing the file)
	
When using the "dpkg" command, the install option (-i or --install) needs to be specified. Requires dependencies won't be installed by using this command.
When using the "apt-get" or "apt" command, the "install" option is required.


Installing with a command line requires to specify the package file.

==========================
2. Post Installation Steps
==========================

After installation, some steps should be done before using the drivers and libraries.
If those steps requires to run a script, it will be located inside the source folder of the package (usually /usr/src/gage-driver-{VERSION}/
The order of the steps is not important.

* Logout of the current account.
	Some global environment variable are required and will take effect only after login in.
	
* Register the driver to DKMS. (Optionnal)
	This can be done using the "register_dkms.sh" script.
	Registering will automatically recompile the driver when there is a kernel update.
	The installation will attempt to register to DKMS if it is already installed.
	Running the script is only required if DKMS was not installed prior to installing the gage driver package.

* Fix the driver loading. (Optionnal)
	This can be done using the "driver.sh" script.
	By default, the system will try to load every driver, either by using the helper script (gagehp.sh -d) or at boot.
	This script will ask which board are present in the computer to only try to load those drivers.


=================
3. Uninstallation
=================

-------
3.1 RPM
-------

Uninstalling can be done using the same 3 methods as installing.
When using command line, the package name must be specified instead of the package file.

When using the rpm command, use the erase option (-e or --erase)
When using the yum command, use the remove option.


-------
3.2 DEB
-------

Uninstalling can be done using the same 3 methods as installing.
When using command line, the package name must be specified instead of the package file.

When using the "dpkg" command, use the erase option (-r or --remove)
When using the "apt-get" or apt command, use the remove option.

