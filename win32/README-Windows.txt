Installation instructions for Windows using WinSDK7 and MinGW Make
------------------------------------------------------------------

Get the source files
--------------------
1. Download the Source for Microsoft Windows (libnfc-1.3.3-winsdk.zip)
2. Extract the files to your computer (e.g. C:\libnfc-1.3.3-winsdk)

MinGW
-----
1. Download MinGW from http://www.mingw.org/
2. Install MinGW Make
3. Make sure to add the MinGW\bin to the Windows PATH Environment Variable

Windows SDK
-----------
1. Download Microsoft Windows SDK v7.0 (http://msdn.microsoft.com/en-us/windows/bb980924.aspx)
2. Open the "Start Menu\All programs\Microsoft Windows SDK v7.0\CMD Shell"

  C:\Program Files\Microsoft SDKs\Windows\v7.0> WindowsSdkVer.exe -version:v7.0
  C:\Program Files\Microsoft SDKs\Windows\v7.0> cd C:\ libnfc-1.3.3-winsdk\win32
  C:\ libnfc-1.3.3-winsdk\win32>mingw32-make

