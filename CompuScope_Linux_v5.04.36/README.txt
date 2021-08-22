RELEASE NOTES

*****************************************************
CompuScope Linux Drivers 5.04.36
*****************************************************

This version of CompuScope Linux Drivers supports the CompuScope PCI 1250X
and all PCI-e CompuScope models: Cobra, CobraMax, Octopus, Octave, Razor,
Oscar, EON Express and RazorMax

The drivers support the following Linux distributions: 
CentOS and Redhat 7.6 (64-bit)
	Kernel v3.10.0 	
Ubuntu 16.04	(64-bit)	
	Kernel v4.15.0	 
Ubuntu 16.04    (32-bit) (except for CompuScope Cobra and CobraMax)
	Kernel v4.4.0

-----------------------------------------------------
ENHANCEMENTS
-----------------------------------------------------

* Support for RedHat and CentOS 7.6 
* Added support for CompuScope RazorMax
* A new GUI application CsTestQt replaces the old VersaScope.
 
-----------------------------------------------------
BUG FIXES
-----------------------------------------------------

* Fixed the problem where Gage driver crashes in some PCs

-----------------------------------------------------
KNOWN PROBLEMS
-----------------------------------------------------

* Expert Streaming does not work in Ubuntu with kernel newer than 4.15.0-29
* The drivers do not support more than 2 CompuScope cards in the same PC.
* CompuScope Cobra or CobraMax does not install with Ubuntu 16.04 (32-bit)
* Gage Resource Manager (csrmd) should be run in background before starting any programs 
  that use CompuScope cards.


-----------------------------------------------------
HOW TO INSTALL THE COMPUSCOPE LINUX DRIVERS
-----------------------------------------------------

IMPORTANT: The steps to build the driver have been changed. Please read carefully and follow the new steps.

Notes: Linux is case sensitive.
       You need Internet connection in order to install CsTestQt or CompuScope driver.
       You need Administrative privilege to install CompuScope driver.
       
------------------------------------------------------------------------------
Only for Red Hat system, follow the steps below before building the driver:
-------------------------------------------------------------------------------

- Boot into Linux.

- From the Linux menu bar select "Applications" ->  "System Tools" -> "Software Update".

- Select "Install Updates"; you will be asked to supply the "root" password.
	- Trust all the packages if any packages have out of date keys.

- When the updates are complete select "Restart Computer" from the new popup.
 
- After the computer boots up, select from the Linux menu bar "Applications" ->"System Tools" -> "Software".

- Select from the Linux menu bar "Software" -> "Software sources".

- Enable all non-debug, non-source repositories.

- Close the "Software sources" popup and then close the "Add/remove software" popup.

------------------------------------------------------------------------------
Only for Ubuntu 32-bit system, follow the steps below before building the driver:
-------------------------------------------------------------------------------
* Edit grub
- sudo gedit /etc/default/grub
- On the line beginning with “GRUB_CMDLINE_LINUX_DEFAULT” and add ‘iommu=soft’ inside the double-quotes, e.g. “quiet splash iommu=soft”
- save the file
- sudo update-grub
- reboot system
- build the driver following the steps described below
-----------------------------------------------------------------------------


- Install the CompuScope card(s).

- Connect a signal to at least the first channel of the CompuScope card.

- Insert the CompuScope Linux Driver CD into the DVD-RW drive.

- Copy all files from the CD to user's home folder.

- Double click on the "gati-linux-driver.tar.gz" file from the user's home folder.

- Select "Extract" from the popup menu bar.

- Select "Extract" from the new popup.

- Select "Close" and then close the gati-linux-driver.tar.gz  popup.

- Go to gati-linux-driver folder under the user's home folder, right click and select "Open in terminal".

- In the terminal window enter the following:
     chmod a+x *.sh

- For CentOs:
  In the terminal window enter the following command:
      ./gagesc.sh install -c <codename> -ad -o redhat

- For RedHat and Ubuntu:
  In the terminal window enter the following command:
      ./gagesc.sh install -c <codename> -ad


For CompuScope 1250X
	- <codename>  = johndeere

For CompuScope Cobra
	- <codename>  = rabbit

For CompuScope CobraMax
	- <codename>  = cobramax

For CompuScope Razor and Oscar
	- <codename>  = splenda

For CompuScope Octopus and Octave
	- <codename>  = spider

For CompuScope EON Express
	- <codename>  = decade

For CompuScope RazorMax
	- <codename>  = hexagon

- You will be prompted for your password during the installation. 

- Once the script completes, run the following commands to build all the SDK sample programs:
     cd Sdk/
     make

- Before running any SDK Sample programs or any programs that use CompuScope cards, you must ensure that Gage Resource Manager (csrmd) is running in background. 
  To check "csrmd" status, use the following command:
     gagehp.sh -i

- Start "csrmd" by using the following command:
     gagehp.sh -c start

-------------------------------------------------------------------------
HOW TO INSTALL CsTestQt
-------------------------------------------------------------------------

The CsTestQt application is included in this driver. It is a GUI application that allows the user
to configure a CompuScope card to acquire data then view the acquired data on the screen.
The application can be used to perform some simple tests and verify functionalities of a CompuScope card.

To install CsTestQt :
- Go to gati-linux-driver folder under the user's home folder, right click and select "Open in terminal".
  In the terminal window enter the following command:
      cd CsTestQt/
      chmod a+x install_cstestqt.sh
      ./install_cstestqt.sh

To run CsTestQt:
   From any terminals, type:
      cstestqt



_____________________________________________

Comments and suggestions can be addressed to:

Project Manager - CompuScope Linux Drivers
Gage Applied Technologies


In North America:
Tel:    800-567-GAGE
Fax:    800-780-8411

Outside North America:
Tel:    (514) 633-7447
Fax:    (514) 633-0770

E-mail:   prodinfo@gage-applied.com
Web site: www.gage-applied.com
