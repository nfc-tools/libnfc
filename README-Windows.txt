Requirements - Running
======================
In order to use the library and run included examples, at least this
dependency should be installed:

- Microsoft Visual C++ 2008 SP1 Redistributable Package [1]

These C++ 2008 SP1 libraries are also part of "Microsoft .NET Framework 3.5 
Service Pack 1 and .NET Framework 3.5 Family" which can be obtained through 
Windows Update. 

There are two versions of the redistributable package (one for x86 and one for 
x64). Be sure which one is needed.

An optional requirement is:

- LibUsb-Win32 0.1.12.2 [2]

In case support for the PN531USB and PN533USB is needed. LibUsb has serious issues 
on Windows systems newer than Windows XP (and on 64 bit). See LibUsb support
mailing lists for more information.

Requirements - Development
==========================
This project uses CMake for creating a Visual Studio project from the 
source files.

The following software was used to create the libnfc distribution:

- CMake 2.6.4 [3]
- Microsoft Windows SDK for Windows 7 (7.0) [4]
- Microsoft .NET framework 3.5 SP1 (Windows Update)
- LibUsb-Win32 0.1.12.2 (only on Windows XP builds) [2]
- NSIS 2.45 (to create the installer) [5]
- 7-Zip 4.65 (for generating source archive) [6]

This was tested on Windows XP SP3, but should work on Windows Vista and 
Windows 7 as well. Even on 64 bit systems.

Building
========
To build the distribution the NMake Makefiles generator of CMake was used. Here 
is an example of how to generate a distribution with the above mentioned 
requirements fulfilled (it is assumed the CMake binaries are in the system 
path, this is optional during installation of CMake):

- Start -> Programs -> Microsoft Windows SDK v7.0 -> CMD shell
- Now it is possible to run CMake and NMake:
  
  C:\Program Files\Microsoft SDKs\Windows\v7.0> cd c:\dev\libnfc-read-only
  C:\dev\libnfc-read-only> mkdir bin
  C:\dev\libnfc-read-only> cd bin
  C:\dev\libnfc-read-only\bin> cmake-gui ..
  
Now you can configure the build. Press "Configure", specify "NMake Makefiles" 
and then you have the opportunity to set some configuration variables. If you
don't want a Debug build change the variable CMAKE_BUILD_TYPE to "Release".

If a non-GUI solution is preferred one can use:

  C:\dev\libnfc-read-only\bin> cmake -G "NMake Makefiles" 
                                     -DCMAKE_BUILD_TYPE=Release ..

Now run NMake:

  C:\dev\libnfc-read-only\bin> nmake package
  
To create the binary package, or:
  
  C:\dev\libnfc-read-only\bin> nmake package_source

To create the source archive.  
  
NMake will create a shared library for Windows (nfc.dll), and
a library file (nfc.lib) to link your applications against. It will compile
the tools against this shared library. The installer will install the 
DLL file in the same directory as the tools as our goal was to be able to 
run and install libnfc as a standard user. In a system wide installation
the DLL should be placed in "windows\system32" or any other directory in 
the (system) path.

If you want your own tools to use libnfc you have to link them with "nfc.lib"
which can be found in the "lib" directory under the installation root. The
header files can be found under "include" in the installation root.

In a "normal" Windows install this would be:
- "C:\Program Files\libnfc-x.x.x\lib"
- "C:\Program Files\libnfc-x.x.x\include"

You also need to copy the "nfc.dll" file to the same directory as your 
application, or install it somewhere in the system path (see above).

CMake can also be used to create the MSVC [7] project files instead of NMake 
Makefiles, after which MSVC can be used to compile the project, create an 
installer, etc.

Building 64 Bit
===============
Building on native 64 bit should work "out of the box".

It is also possible to "cross compile" for 64 bit systems on a 32 bit system.
This can be accomplished by using the "setenv" command from the shell opened
in the previous section. Try "setenv /?" to see what is available. When cross 
compiling the name of the binary installer package generated is not correct, 
because CMake does not notice it is cross compiling. It should work fine 
however.

References
==========
[1] http://www.microsoft.com/downloads/details.aspx?familyid=A5C84275-3B97-4AB7-A40D-3802B2AF5FC2
[2] http://libusb-win32.sourceforge.net/
[3] http://www.cmake.org
[4] http://msdn.microsoft.com/en-us/windows/bb980924.aspx
[5] http://nsis.sourceforge.net/Main_Page
[6] http://http://www.7-zip.org/
[7] http://www.microsoft.com/express/vc/
