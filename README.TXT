------------------------------------------------------------------------
Public platform independent Near Field Communication (NFC) library
Copyright (C) 2009, Roel Verdult
 
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

------------------------------------------------------------------------
Welcome to the community of libnfc (website: http://www.libnfc.org).

libnfc version 1.0.0

Index:
1. Introduction
2. Hardware
3. Compilation
4. Examples
5. Todo
6. Contact

------------------------------------------------------------------------
1. Introduction

Since the RFID market is spoiled by proprietary hard and software
we want to contribute constructively by distributing a free library
which can be used in RFID/NFC applications.

------------------------------------------------------------------------
2. Hardware

To communicate with NFC targets/tags you need a USB device which is
called a "RFID Reader". The cheapest hardware around that can carries
a fully NFC complied chip is the ACR122 from the company ACS, Advanced 
Card Systems Limited (website: http://www.acs.com.hk/acr122.php).
This reader uses the PN532 chipset from NXP Semiconductors. The reader
costs around the €60 and is available in different webshops all over
the world. The NFC reader used in the "tikitag" project is exactly the
same reader as the ACR122 but with a different logo. This reader is
available for only €35,- (website: http://www.tikitag.com). We would
recommend to use one of these readers, since we tested both of them.

Other NFC readers, like the SCL3710 from SCM Microsystems (website: 
http://www.scmmicro.com/scl3710/) are not supported yet. They use the
same chipset, so the instruction set is completely the same. This means
that with little effort these readers could be added to be part of the
NFC library. We would be very pleased if someone is willing to add this
support to libnfc.

------------------------------------------------------------------------
3. Compilation

If you use linux, please make sure you have libpcsc-lite installed.
For Ubuntu this means you have to install the following packages:
$sudo apt-get install pcscd
$sudo apt-get install libpcsclite-dev
When these are installed, just run "make" and you are ready to go.

MacOSX users have to edit the "Makefile" file in the main directory.
Uncomment the following lines:
#LIB_TYPE=...
#LIB_FLAGS=...
#PCSC_HEADERS=...
#PCSC_LIB=...
Run "make" and you it should work like a charm.

Windows users can use the binairy files located in the "win32bin"
directory. Or they can open the VS2005 solution which can be found
in the "win32" directory. Compilation should work "out of the box".

------------------------------------------------------------------------
4. Examples

There are 6 example tools coming with this library. They can be used
as a reference during development of your software. All tools try to
demonstrate a different feature. Below there is short description of
the tools and their purpose.

anticol  - Anti-collision demonstration tool for ISO14443A tags using
           self constructed REQA/WUPA and SELECT frames.

list     - List passive tags tool for ISO14443A tags using the commands
           available in the instruction set of the hardware chip.

simulate - With this example you can turn your reader into a NFC target.
           For testing purposes it simulates a ISO14443A tag with a UID
           that consists of 0xDEADBEAF. The frames are delayed because
           of the USB communication, so not all readers will recognize
           the tag. The "Omnikey 5121" though has no problems with this
           small delay.

relay    - This tool allows the user to use 2 readers. One will similate
           a ISO14443A tag, while the 2nd reader will stay a reader.
           The original tag can be placed on the 2nd reader and the
           simulated tag can be placed on top of the original reader.
           All communication is relayed now and showed on the screen.
           NOTE: relaying frames is time-consuming, not all readers
           allow slow responds from the (simulated) tag. Though, one of 
           the most used RFID readers, the "Omnikey 5121" has no problem
           and can perfectly be used for demonstrating this features.

mfread   - With this tool a complete MIFARE card can be readout and stored
           in a MIFARE dump file (*.mfd).

mfwrite  - With this tool a complete MIFARE card can be restored from a
           a MIFARE dump file (*.mfd) back to the tag.

A MIFARE dump file (*.mfd) is a plain 4KB image of a mifare tag. This
image should contain the data, keys and access bits on their usual position.
A Dump file is always 4KB, even when a 1KB tag is dumped. From the first
block, the manufacturer data, can be determined if you are dealing with
a 1KB or a 4KB tag. This is stored in the ATQA part of block0 (byte 6/7).
More info can be found in the MIFARE functional specification from NXP.
(Website: http://www.nxp.com/products/identification/mifare/classic)

------------------------------------------------------------------------
5. Todo

Support more communication protocols. Sony Felica, ISO14443B and Innovision
Jewel Topaz tags are supported by the chipset, but they are not used by
this library. It would be very useful if we could add support for these
tags in this library.

------------------------------------------------------------------------
6. Contact

Please visit the website (http://www.libnfc.org) and place your questions
in the community forum there. We are sure a member of the community will 
help you out.

------------------------------------------------------------------------
Proprietary notes:

Note: FeliCa is s registered trademark of Sony Corporation. MIFARE is a 
trademark of NXP Semiconductors. Jewel Topaz is a trademark of Innovision
Research & Technology. All other trademarks are the property of their 
respective owners.
