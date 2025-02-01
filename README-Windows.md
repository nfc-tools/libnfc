    *-
    * Free/Libre Near Field Communication (NFC) library
    *
    * Libnfc historical contributors:
    * Copyright (C) 2009      Roel Verdult
    * Copyright (C) 2009-2013 Romuald Conty
    * Copyright (C) 2010-2012 Romain Tartière
    * Copyright (C) 2010-2013 Philippe Teuwen
    * Copyright (C) 2012-2013 Ludovic Rousseau
    * Additional contributors of Windows-specific parts:
    * Copyright (C) 2010      Glenn Ergeerts
    * Copyright (C) 2013      Alex Lian
    -*

Requirements
============

- MinGW-w64 compiler toolchain [1]
- LibUsb-Win32 1.2.5.0 (or greater) [2]
- CMake 2.8 [3]

This was tested on Windows 7 64 bit, but should work on Windows Vista and
Windows XP and 32 bit as well.
Only the ACS ACR122 and the ASK Logo readers are tested at the moment, so any feedback about other devices is very welcome.

Community forum: http://www.libnfc.org/community/

Building
========

To build the distribution the MinGW Makefiles generator of CMake was used. Here
is an example of how to generate a distribution with the above mentioned
requirements fulfilled (it is assumed the CMake binaries are in the system
path, this is optional during installation of CMake):

- Add the following directories to your PATH:

        c:\MinGW64\bin;c:\MinGW64\x86_64-w64-mingw32\lib32;c:\MinGW64\x86_64-w64-mingw32\include

- Now it is possible to run CMake and mingw32-make:

        C:\dev\libnfc-read-only> mkdir ..\libnfc-build
      C:\dev\libnfc-read-only> cd ..\libnfc-build
      C:\dev\libnfc-build> cmake-gui .

Now you can configure the build. Press "Configure", specify "MinGW32 Makefiles"
and then you have the opportunity to set some configuration variables. If you
don't want a Debug build change the variable CMAKE_BUILD_TYPE to "Release".

If a non-GUI solution is preferred one can use:

    C:\dev\libnfc-build> cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
                                     -DLIBNFC_CONFDIR="./config" ..\libnfc-read-only

Now run mingw32-make to build:

    C:\dev\libnfc-read-only\bin> mingw32-make

The build will create a shared library for Windows (nfc.dll) to link your applications against. It will compile
the tools against this shared library.

References
==========
[1] the easiest way is to use the TDM-GCC installer.
        Make sure to select MinGW-w64 in the installer, the regular MinGW does not contain headers for PCSC.
        http://sourceforge.net/projects/tdm-gcc/files/TDM-GCC%20Installer/tdm64-gcc-4.5.1.exe/download

[2] http://sourceforge.net/projects/libusb-win32/files/

[3] http://www.cmake.org
