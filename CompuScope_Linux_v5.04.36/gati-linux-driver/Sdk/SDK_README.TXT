PRELIMINARY

Gage Applied Technologies

The Gage C\C++ SDK is extracted to an Sdk folder with the following subdirectory structure:

Sdk
   Advanced		(folder for advanced samples)
   C_Common		(shared source code files)
   CsAppSupport		(shared library used by SDK examples)
   GageAcquire
   GageCoerce
   GageComplexTrigger
   GageDeepAcquisition
   GageMultipleRecord
   GageMultipleSystems

The Sdk examples depend on shared libraries and common files which should have been extracted and compiled with the driver files. In addition, the driver makefile will compile CsAppSupport (libCsAppSupport.so) and copy it to the /usr/local/lib directory.

There is a master make file in the Sdk directory which can be used to build, and clean, all the Sdk samples.

Most of the Sdk samples consist of a their own C file and a shared C file (CsSdkMisc.c)
with common routines. In addition the samples all use the libCsAppSupport.so shared library for common routines. The source code for this library is in CsAppSupport.


