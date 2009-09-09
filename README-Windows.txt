Requirements
============
This project uses CMake [1] to create a Visual Studio project from the 
source files. 

It have been tested with:

- Microsoft Visual C++ 2008 Express Edition, with SP1 [2]
- LibUsb-Win32 0.1.12.2 [3]
- NSIS 2.45 (to create Windows installer) [4]

On Windows XP SP3.

Compilation
===========

CMake can be used to create MSVC project files, then each MSVC can 
be used to compile the project, create an installer, etc.

MSVC project will create a shared library for Windows (nfc.dll), and
a library file (nfc.lib) to link your applications against. It will compile
examples against this shared library. Installer will install the DLL file 
in the same directory as the examples if your goal was to be able to 
run and install libnfc as a standard user. In a system wide installation
the DLL should be placed in "windows\system32" or any other directory in 
Windows PATH.

If you want your own tools to use libnfc you have to link them with "nfc.lib"
which can be found in "lib" subdirectory under the installation root. Headers 
can be found in "include" subdirectory.

In a "normal" Windows install this would be:
- "C:\Program Files\libnfc-x.x.x\lib"
- "C:\Program Files\libnfc-x.x.x\include"



[1] http://www.cmake.org
[2] http://www.microsoft.com/express/vc/
[3] http://libusb-win32.sourceforge.net/
[4] http://nsis.sourceforge.net/Main_Page
