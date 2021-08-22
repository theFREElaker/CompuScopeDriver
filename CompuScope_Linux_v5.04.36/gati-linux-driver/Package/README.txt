=================
Table of Contents
=================

1. Introduction
2. Required Files
	2.1 Common files
	2.2 Red Hat Files
	2.3 Debian Files
3. Building Steps
	3.1 Red Hat Steps
	3.2 Debian Steps

===============
1. Introduction
===============

The "Package" contains the required files to generate a Red Hat or a Debian package file.
Specific files needed by the Red Hat or Debian packaging are located with their sub-folder, "rpm" and "deb" respectively.


=================
2. Required Files
=================

----------------
2.1 Common Files
----------------

build_modules.sh
	Script used to compile the Kernel Module (driver).
	The file is relocated to the root Gage folder in order to be usable (within /usr/src/gage-driver-x.x.x/ in normal installs).
	
clean_modules.sh
	Script used to remove intermediary  compilation files and uninstalled compiled files.
	The file is relocated to the root Gage folder in order to be usable.
	
dkms.conf
	Configuration file used by DKMS. This file is only used if DKMS is installed on the system.
	The file is relocated to the root Gage folder in order to be usable.
	
driver.sh
	Script used to change drivers in the XML file and the  drivers loaded at boot.
	This script will create a backup of the files being modified if they don't exists.
	
gage.conf
	Configuration files used by ldconfig. Specifies the path to the Gage library files to be used at runtime.
	The file is relocated to "/etc/ld.so.conf.d/" in order to be usable.
	Calling "ldconfig" is required in order to refresh the available libraries.
	
gage.sh
	Script used to set or update the "LIBRARY_PATH" environment variable used by GCC and start the resource manager.
	The file is relocated to "/etc/profile.d/" in order to be usable.
	Red Hat systems can run the script by calling "source /etc/profile".
	Ubuntu systems requires the user to logout of their account before the environment variable is properly set.

gagehp.sh
	Helper script used for to start/stop the resource mangaer, insert/remove kernel drivers or get information about the current state of the resource manager and drivers
	Has some slight modification from the usual helper script to accomodate the different path used by the install.

gage-driver.conf
	File used by the system to load the gage drivers at boot.
	Must be installed at "/etc/modules-load.d/".
	
register_dkms.sh
	Script used to install DKMS dependencies and register the Gage Driver to DKMS.
	Due to OS restriction, the installation cannot be done when installing the Gage Driver, in which case the user will have to run the script afterwards.

-----------------
2.2 Red Hat Files
-----------------

gage-driver.spec
	The specification file used to create the RPM archive.
	Sets configuration parameters, install directories.
	Contains post-install and pre-remove scripts.

----------------
2.3 Debian Files
----------------

For any debian file, the macro #VERSION# can be used to insert the current version being built.
The macro #TIME# can be used to insert the current date and time.

changelog
	The changelog. The most recent change is at the beginning of the file.
	The most recent log entry version number must match the version being packaged.
	This file must be manually edited.

compat
	Defines the debhelper compatibility level. Should contain "9".

control
	Configuration file for the package manager.
	Package dependencies should go in this file.

copyright
	File containing the copyright information for every file in the source tree.

gage-driver.postinst
	The post-installation script. Creates the required XML file, compiles and installs the kernel module.
	Will try to register the drivers to DKMS.
	
gage-driver.preinst
	The pre-installation script. Checks if the linux headers for the current kernel are installed.
	Will attempt to install the headers if they are not installed.

gage-driver.prerm
	The pre-remove script. Uninstall the kernel modules. Unregister the modules from DKMS.

README.debian
	A readme file for the packaging.

rules
	The makefile used to create the package.
	Targets will usually call a "dh" command unless the target is specifically written.
	The "dh" commands will call various commands (such as "dh_install"). Those command can be overwritten by having a new target with the "override_" prefix (example, override_dh_install)

=================
3. Building Steps
=================

Both Red Hat and Debian packaging can be done using the "gagesc.sh" script.
Red Hat packaging can be invoked with the "buildrpm" command when running the script.
Debian packaging can be invoked with the "builddeb" command when running the script.

Required dependencies can be installed using the "-d" flag when invoking a command from the script.

-----------------
3.1 Red Hat Steps
-----------------

1- Create an archive containing the source code of the module.
	This archive is created using the "distcheck" target from the root Makefile.

2- Creating the build tree.
	This tree contains directories for building, storing the installation structure and will contain the Red Hat package.
	The location is of low importance, but the sub-directories must be present

3- Invoke the rmpbuild command.
	The command to create the package.
	Requires to specify the path to the build tree and the path to the source code archive.

----------------
3.2 Debian Steps
----------------

1- Create an archive containing the source code of the module.
	This archive is created using the "distcheck" target from the root Makefile.

2- Create the build tree.
	The location is of low importance. The inner structure will be built at later steps.
	E.g. "gati-linux-driver/Package/deb/output"
	
3- Unpack the archive in a sub-directory in the build tree.
	The name must follow this convention : "gage-driver-<version>". Where <version> is the current gage-driver version being built.
	E.g. "gati-linux-driver/Package/deb/output/gage-driver-5.0.63"
	
4- Copy the debian template files within the "debian" sub-directory in the unpacked source code
	E.g. "gati-linux-driver/Package/deb/output/gage-driver-5.0.63/debian/"
	Version number and timestamps are also added to required files by replacing a macro within those files.
	
5- Create the makefiles using "./configure"

6- Build the package.
	The tool used to build the package is "debuild".
	The options "-us -uc" are to disable GPG signing on the package.
	Those options can be removed, but the user will need to have a valid GPG signature file in "*** Insert path ***".
	
7- Delete the driver source from the build tree.
	Optionnal step. This source will not be used anymore and can be deleted.
