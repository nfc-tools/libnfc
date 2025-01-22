```
*-
* Free/Libre Near Field Communication (NFC) library
*
* Libnfc historical contributors:
* Copyright (C) 2009      Roel Verdult
* Copyright (C) 2009-2015 Romuald Conty
* Copyright (C) 2010-2012 Romain Tartière
* Copyright (C) 2010-2013 Philippe Teuwen
* Copyright (C) 2012-2013 Ludovic Rousseau
* Additional contributors:
* See AUTHORS file
-*
```

General Information
===================

libnfc is a library which allows userspace application access to NFC devices.

The official web site is:
  http://www.nfc-tools.org/

The official forum site is:
  http://www.libnfc.org/community/

The official development site is:
  https://github.com/nfc-tools/libnfc

Important note: this file covers POSIX systems, for Windows please read README-Windows.md

Requirements
============

Some NFC drivers depend on third party software:

* pn53x_usb & acr122_usb:
  
  - libusb-0.1 http://libusb.sf.net

* acr122_pcsc:
  
  - pcsc-lite https://pcsclite.apdu.fr/
- pcsc:
  
  - Support build with pcsc driver, which can be using all compatible readers, Feitian R502 and bR500 already passed the test.

The regression test suite depends on the cutter framework:
http://cutter.sf.net

Building
========

Note: If working directly from a git clone of the repository, some of the files need to be generated first. To do this run
`autoreconf -vis`

Alternatively use a .tar.bz2 version of a packaged release (which already contains ./configure):
https://github.com/nfc-tools/libnfc/releases/

The build should be as simple as running these commands:

    ./configure
    make


To build with specific driver(s), see option `--with-drivers=...` detailed in `./configure --help`.

Installation
============
    
    make install

You may need to grant permissions to your user to drive your device.
Under GNU/Linux systems, if you use udev, you could use the provided udev rules.
  e.g. under Debian, Ubuntu, etc.

    sudo cp contrib/udev/93-pn53x.rules /lib/udev/rules.d/

Under FreeBSD, if you use devd, there is also a rules file: contrib/devd/pn53x.conf.

Configuration
=============

In order to change the default behavior of the library, the libnfc uses a
configuration file located in sysconfdir (as provided to ./configure).

A sample commented file is available in sources: libnfc.conf.sample

If you have compiled using:

    ./configure --prefix=/usr --sysconfdir=/etc

you can make configuration directory and copy the sample file:

    sudo mkdir /etc/nfc
    sudo cp libnfc.conf.sample /etc/nfc/libnfc.conf

To configure multiple devices, you can either modify libnfc.conf or create a
file per device in a nfc/devices.d directory:

    sudo mkdir -p /etc/nfc/devices.d
    printf 'name = "My first device"\nconnstring = "pn532_uart:/dev/ttyACM0"\n' | sudo tee /etc/nfc/devices.d/first.conf
    printf 'name = "My second device"\nconnstring = "pn532_uart:/dev/ttyACM1"\n' | sudo tee /etc/nfc/devices.d/second.conf

Environment Variables
=====================
You can override certain configuration options at runtime using the following environment variables:
+ `LIBNFC_DEFAULT_DEVICE=<connstring>`:  `LIBNFC_DEFAULT_DEVICE=pn532_uart:/dev/ttyACM0` will use pn532 on /dev/ttyACM0 as default device
+ `LIBNFC_DEVICE=<connstring>` will ignore all devices in the config files and use only the one defined in the variable
+ `LIBNFC_AUTO_SCAN=<true|false>` overrides `allow_autoscan` option in the config file
+ `LIBNFC_INTRUSIVE_SCAN=<true|false>` overrides `allow_intrusive_scan` option in the config file
+ `LIBNFC_LOG_LEVEL=<0|1|2|3>` overrides `log_level` option in the config file

To obtain the connstring of a recognized device, you can use `nfc-scan-device`: `LIBNFC_AUTO_SCAN=true nfc-scan-device` will show the names & connstrings of all found devices.

How to report bugs
==================

To report a bug, visit https://github.com/nfc-tools/libnfc/issues and fill
out a bug report form.

If you have questions, remarks, we encourage you to post this in the developers
community:
http://www.libnfc.org/community

Please make sure to include:

* The version of libnfc

* Information about your system. For instance:
  
  - What operating system and version
  - For Linux, what version of the C library
  
  And anything else you think is relevant.

* A trace with debug activated.
  
  Reproduce the bug with debug, e.g. if it was:
  
        $ nfc-list -v
  
  run it as:
  
        $ LIBNFC_LOG_LEVEL=3 nfc-list -v

* How to reproduce the bug.
  
  Please include a short test program that exhibits the behavior.
  
  As a last resort, you can also provide a pointer to a larger piece
  
  of software that can be downloaded.

* If the bug was a crash, the exact text that was printed out
  
  when the crash occured.

* Further information such as stack traces may be useful, but
  
  is not necessary.

Patches
=======

Patches can be posted to https://github.com/nfc-tools/libnfc/issues

If the patch fixes a bug, it is usually a good idea to include
all the information described in "How to Report Bugs".

Troubleshooting
===============

Touchatag/ACR122:
-----------------

If your Touchatag or ACR122 device fails being detected by libnfc, make sure
that PCSC-lite daemon (`pcscd`) is installed and is running.

If your Touchatag or ACR122 device fails being detected by PCSC-lite daemon
(`pcsc_scan` doesn't see anything) then try removing the bogus firmware detection
of libccid: edit libccid_Info.plist configuration file (usually
`/etc/libccid_Info.plist`) and locate `<key>ifdDriverOptions</key>`, turn
`<string>0x0000</string>` value into `0x0004` to allow bogus devices and restart
pcscd daemon.

ACR122:
-------

Using an ACR122 device with libnfc and without tag (e.g. to use NFCIP modes or
card emulation) needs yet another PCSC-lite tweak: You need to allow usage of
CCID Exchange command.  To do this, edit `libccid_Info.plist` configuration file
(usually `/etc/libccid_Info.plist`) and locate `<key>ifdDriverOptions</key>`,
turn `<string>0x0000</string>` value into `0x0001` to allow CCID exchange or
`0x0005` to allow CCID exchange and bogus devices (cf previous remark) and
restart pcscd daemon.

Warning: if you use ACS CCID drivers (acsccid), configuration file is located
in something like: `/usr/lib/pcsc/drivers/ifd-acsccid.bundle/Contents/Info.plist`

SCL3711:
--------

Libnfc cannot be used concurrently with the PCSC proprietary driver of SCL3711.
Two possible solutions:

* Either you don't install SCL3711 driver at all
* Or you stop the PCSC daemon when you want to use libnfc-based tools

PN533 USB device on Linux >= 3.1:
---------------------------------

Since Linux kernel version 3.1, a few kernel-modules must not be loaded in order
to use libnfc : "nfc", "pn533" and "pn533_usb".
To prevent kernel from loading automatically these modules, you can blacklist
them in a modprobe conf file. This file is provided within libnfc archive:

    sudo cp contrib/linux/blacklist-libnfc.conf /etc/modprobe.d/blacklist-libnfc.conf

FEITIAN bR500 and R502:
-----------------------

Libnfc can work with PCSC proprietary driver of bR500 and R502, which is already available on most Linux setups.
To activate the PCSC support: `./configure --with-drivers=pcsc`.
Readers known to work:

- Feitian bR500
- Feitian R502 Dual interface reader
- Feitian R502 CL(Contactless) reader

These readers are support by CCID since v1.4.25, make sure your CCID driver version higher or equal to 1.4.25.

On macOS, you can check your CCID version with the following command, and if required, you can install latest CCID driver from [https://github.com/martinpaljak/osx-ccid-installer/releases](https://github.com/martinpaljak/osx-ccid-installer/releases)

```
grep -A 1 CFBundleShortVersionString /usr/local/libexec/SmartCardServices/drivers/ifd-ccid.bundle/Contents/Info.plist
```

On Linux, you can check your CCID version with the following command, and if required, you can install latest CCID driver from [https://ccid.apdu.fr/](https://ccid.apdu.fr/)

```
grep -A 1 CFBundleShortVersionString /usr/lib/pcsc/drivers/ifd-ccid.bundle/Contents/Info.plist
```

Proprietary Notes
=================
FeliCa is a registered trademark of the Sony Corporation.
MIFARE is a trademark of NXP Semiconductors.
Jewel Topaz is a trademark of Innovision Research & Technology.
All other trademarks are the property of their respective owners.
