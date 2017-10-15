## Compile for Windows - 64-BIT (x64)  
   
#### Tested using windows 10 pro x64, build 1703.  
  
Install latest libusbk release from: (this installs both libusb and libusbk windows components required for running libnfc applications)  
https://sourceforge.net/projects/libusb-win32/files/libusbK-release/  
https://sourceforge.net/projects/libusb-win32/files/libusbK-release/libusbK-3.0.7.0-setup.exe  
  
#### install mingw64 toolchain on Windows:  
  
##### Install latest version of: MSYS2  for 64-bit windows. 
##### even if msys2 version appears old, the packages it installs are very recent:  
http://www.msys2.org/  
 msys2-x86_64-xxxxxxxx.exe  
  
  
##### Open the MSYS shell in mingw-64 mode (64-bit mode)  
```  
#update the MSYS system using pacman:  
pacman -Syu  

#close the shell, re-open and run:  
pacman -Su   
  
#install build tools:  
pacman -S git make mingw-w64-$(uname -m)-gcc mingw-w64-$(uname -m)-binutils mingw-w64-$(uname -m)-cmake mingw-w64-$(uname -m)-zlib mingw-w64-$(uname -m)-make  
```  
  
#### download and extract: libusb-win32-bin-1.2.6.0.zip   
##### todo: (this step shouldn't be needed, as libusb0.dll is already in system32, but the logic is broken in the libusb search that cmake uses in: cmake/modules/FindLIBUSB.cmake, so do this step for now)    
https://sourceforge.net/projects/libusb-win32/files/libusb-win32-releases/1.2.6.0/  
##### Extract contents to the native program files directory: (not the x86 program files!):   
C:\Program Files\libusb-win32  

#### Restart msys2 bash - in mingw-64 mode.  
  
```

#download libnfc source files:  
cd ~  
mkdir libnfc-build-win64  
git clone https://github.com/nfc-tools/libnfc.git
    
# launch windows powershell x64 (non-admin)  
# add some paths for current build session:  
  
$env:Path += ";c:\msys64\mingw64\bin;c:\msys64\mingw64\x86_64-w64-mingw32\lib;c:\msys64\mingw64\x86_64-w64-mingw32\include"
cd C:\msys64\home\<your username>\libnfc-build-win64  

#then, run "CMD.exe" from within the powershell, (this ensures the PATH set above is available in the build environment)
cmd.exe  

# configure the build with cmake:
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..\libnfc  
  
# run cmake again, same parameters, otherwise errors appear:
cmake -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release ..\libnfc  
  
# build it!
mingw32-make  

```
#### put your "libnfc.conf" into c:\program files (x86)\libnfc\config\libnfc.conf   
todo: it is the wrong location currently for a x64 system, but it works and this will be fixed to use c:\programdata\libnfc\config\libnfc.conf at some point.  
  
#### put compiled DLL file to the real system32 directory: (c:\windows\sysnative\libnfc.dll on syswow64 machines)   
copy .\libnfc\libnfc.dll c:\windows\system32\libnfc.dll  

#### Run the executables found in libnfc\utils or libnfc\examples 

